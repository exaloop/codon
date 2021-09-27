#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <gc.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "parser/common.h"
#include "parser/parser.h"
#include "seq/seq.h"
#include "sir/llvm/llvisitor.h"
#include "sir/transform/manager.h"
#include "sir/transform/pass.h"
#include "sir/util/inlining.h"
#include "sir/util/irtools.h"
#include "sir/util/outlining.h"
#include "util/common.h"
#include "gtest/gtest.h"

using namespace seq;
using namespace std;

class TestOutliner : public ir::transform::OperatorPass {
  int successes = 0;
  int failures = 0;
  ir::ReturnInstr *successesReturn = nullptr;
  ir::ReturnInstr *failuresReturn = nullptr;

  const std::string KEY = "test-outliner-pass";
  std::string getKey() const override { return KEY; }

  void handle(ir::SeriesFlow *v) override {
    auto *M = v->getModule();
    auto begin = v->begin(), end = v->end();
    bool sawBegin = false, sawEnd = false;
    for (auto it = v->begin(); it != v->end(); ++it) {
      if (ir::util::isCallOf(*it, "__outline_begin__") && !sawBegin) {
        begin = it;
        sawBegin = true;
      } else if (ir::util::isCallOf(*it, "__outline_end__") && !sawEnd) {
        end = it;
        sawEnd = true;
      }
    }
    if (sawBegin && sawEnd) {
      auto result = ir::util::outlineRegion(ir::cast<ir::BodiedFunc>(getParentFunc()),
                                            v, begin, end);
      ++(result ? successes : failures);
      if (successesReturn)
        successesReturn->setValue(M->getInt(successes));
      if (failuresReturn)
        failuresReturn->setValue(M->getInt(failures));
    }
  }

  void handle(ir::ReturnInstr *v) override {
    auto *M = v->getModule();
    if (getParentFunc()->getUnmangledName() == "__outline_successes__") {
      v->setValue(M->getInt(successes));
      successesReturn = v;
    }
    if (getParentFunc()->getUnmangledName() == "__outline_failures__") {
      v->setValue(M->getInt(failures));
      failuresReturn = v;
    }
  }
};

class TestInliner : public ir::transform::OperatorPass {
  const std::string KEY = "test-inliner-pass";
  std::string getKey() const override { return KEY; }

  void handle(ir::CallInstr *v) override {
    auto *M = v->getModule();
    auto *f = ir::cast<ir::BodiedFunc>(ir::util::getFunc(v->getCallee()));
    auto *neg = M->getOrRealizeMethod(M->getIntType(), ir::Module::NEG_MAGIC_NAME,
                                      {M->getIntType()});
    if (!f)
      return;
    auto name = f->getUnmangledName();
    if (name.find("inline_me") != std::string::npos) {
      auto aggressive = name.find("aggressive") != std::string::npos;
      auto res = ir::util::inlineCall(v, aggressive);
      if (!res)
        return;
      for (auto *var : res.newVars)
        ir::cast<ir::BodiedFunc>(getParentFunc())->push_back(var);
      v->replaceAll(ir::util::call(neg, {res.result}));
    }
  }
};

vector<string> splitLines(const string &output) {
  vector<string> result;
  string line;
  istringstream stream(output);
  const char delim = '\n';

  while (getline(stream, line, delim))
    result.push_back(line);

  return result;
}

static pair<bool, string> findExpectOnLine(const string &line) {
  for (auto EXPECT_STR : vector<pair<bool, string>>{
           {false, "# EXPECT: "}, {false, "#: "}, {true, "#! "}}) {
    size_t pos = line.find(EXPECT_STR.second);
    if (pos != string::npos)
      return {EXPECT_STR.first, line.substr(pos + EXPECT_STR.second.length())};
  }
  return {false, ""};
}

static pair<vector<string>, bool> findExpects(const string &filename, bool isCode) {
  vector<string> result;
  bool isError = false;
  string line;
  if (!isCode) {
    ifstream file(filename);
    if (!file.good()) {
      cerr << "error: could not open " << filename << endl;
      exit(EXIT_FAILURE);
    }

    while (getline(file, line)) {
      auto expect = findExpectOnLine(line);
      if (!expect.second.empty()) {
        result.push_back(expect.second);
        isError |= expect.first;
      }
    }
    file.close();
  } else {
    istringstream file(filename);
    while (getline(file, line)) {
      auto expect = findExpectOnLine(line);
      if (!expect.second.empty()) {
        result.push_back(expect.second);
        isError |= expect.first;
      }
    }
  }
  return {result, isError};
}

string argv0;

class SeqTest
    : public testing::TestWithParam<tuple<
          string /*filename*/, bool /*debug*/, string /* case name */,
          string /* case code */, int /* case line */, bool /* barebones stdlib */>> {
  vector<char> buf;
  int out_pipe[2];
  pid_t pid;

public:
  SeqTest() : buf(65536), out_pipe(), pid() {}
  string getFilename(const string &basename) {
    return string(TEST_DIR) + "/" + basename;
  }
  int runInChildProcess() {
    assert(pipe(out_pipe) != -1);
    pid = fork();
    GC_atfork_prepare();
    assert(pid != -1);

    if (pid == 0) {
      GC_atfork_child();
      dup2(out_pipe[1], STDOUT_FILENO);
      close(out_pipe[0]);
      close(out_pipe[1]);

      auto file = getFilename(get<0>(GetParam()));
      auto code = get<3>(GetParam());
      auto startLine = get<4>(GetParam());
      auto *module = parse(argv0, file, code, !code.empty(),
                           /* isTest */ 1 + get<5>(GetParam()), startLine);
      if (!module)
        exit(EXIT_FAILURE);

      ir::transform::PassManager pm;
      Seq seqDSL;
      seqDSL.addIRPasses(&pm, /*debug=*/false); // always add all passes
      pm.registerPass(std::make_unique<TestOutliner>());
      pm.registerPass(std::make_unique<TestInliner>());
      pm.run(module);

      ir::LLVMVisitor visitor(/*debug=*/get<1>(GetParam()));
      visitor.visit(module);
      visitor.run({file});

      fflush(stdout);
      exit(EXIT_SUCCESS);
    } else {
      GC_atfork_parent();
      int status = -1;
      close(out_pipe[1]);
      assert(waitpid(pid, &status, 0) == pid);
      read(out_pipe[0], buf.data(), buf.size() - 1);
      close(out_pipe[0]);
      return status;
    }
    return -1;
  }
  string result() { return string(buf.data()); }
};
static string
getTestNameFromParam(const testing::TestParamInfo<SeqTest::ParamType> &info) {
  const string basename = get<0>(info.param);
  const bool debug = get<1>(info.param);

  // normalize basename
  // size_t found1 = basename.find('/');
  // size_t found2 = basename.find('.');
  // assert(found1 != string::npos);
  // assert(found2 != string::npos);
  // assert(found2 > found1);
  // string normname = basename.substr(found1 + 1, found2 - found1 - 1);
  string normname = basename;
  replace(normname.begin(), normname.end(), '/', '_');
  replace(normname.begin(), normname.end(), '.', '_');
  return normname + (debug ? "_debug" : "");
}
static string
getTypeTestNameFromParam(const testing::TestParamInfo<SeqTest::ParamType> &info) {
  return getTestNameFromParam(info) + "_" + get<2>(info.param);
}
TEST_P(SeqTest, Run) {
  const string file = get<0>(GetParam());
  int status;
  bool isCase = !get<2>(GetParam()).empty();
  if (!isCase)
    status = runInChildProcess();
  else
    status = runInChildProcess();
  ASSERT_TRUE(WIFEXITED(status));

  string output = result();

  auto expects = findExpects(!isCase ? getFilename(file) : get<3>(GetParam()), isCase);
  if (WEXITSTATUS(status) != int(expects.second))
    fprintf(stderr, "%s\n", output.c_str());
  ASSERT_EQ(WEXITSTATUS(status), int(expects.second));
  const bool assertsFailed = output.find("TEST FAILED") != string::npos;
  EXPECT_FALSE(assertsFailed);
  if (assertsFailed)
    std::cerr << output << std::endl;

  if (!expects.first.empty()) {
    vector<string> results = splitLines(output);
    for (unsigned i = 0; i < min(results.size(), expects.first.size()); i++)
      if (expects.second)
        EXPECT_EQ(results[i].substr(0, expects.first[i].size()), expects.first[i]);
      else
        EXPECT_EQ(results[i], expects.first[i]);
    EXPECT_EQ(results.size(), expects.first.size());
  }
}
auto getTypeTests(const vector<string> &files) {
  vector<tuple<string, bool, string, string, int, bool>> cases;
  for (auto &f : files) {
    bool barebones = false;
    string l;
    ifstream fin(string(TEST_DIR) + "/" + f);
    string code, testName;
    int test = 0;
    int codeLine = 0;
    int line = 0;
    while (getline(fin, l)) {
      if (l.substr(0, 3) == "#%%") {
        if (line)
          cases.emplace_back(make_tuple(f, true, to_string(line) + "_" + testName, code,
                                        codeLine, barebones));
        auto t = ast::split(l.substr(4), ',');
        barebones = (t.size() > 1 && t[1] == "barebones");
        testName = t[0];
        code = l + "\n";
        codeLine = line;
        test++;
      } else {
        code += l + "\n";
      }
      line++;
    }
    if (line)
      cases.emplace_back(make_tuple(f, true, to_string(line) + "_" + testName, code,
                                    codeLine, barebones));
  }
  return cases;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    TypeTests, SeqTest,
    testing::ValuesIn(getTypeTests({
      "parser/simplify_expr.seq",
      "parser/simplify_stmt.seq",
      "parser/typecheck_expr.seq",
      "parser/typecheck_stmt.seq",
      "parser/types.seq",
      "parser/llvm.seq"
    })),
    getTypeTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    CoreTests, SeqTest,
    testing::Combine(
      testing::Values(
        "core/helloworld.seq",
        // "core/llvmops.seq",
        "core/arithmetic.seq",
        "core/parser.seq",
        "core/generics.seq",
        "core/generators.seq",
        "core/exceptions.seq",
        "core/big.seq",
        "core/containers.seq",
        "core/trees.seq",
        "core/range.seq",
        "core/bltin.seq",
        "core/arguments.seq",
        "core/match.seq",
        "core/kmers.seq",
        "core/formats.seq",
        "core/proteins.seq",
        "core/align.seq",
        "core/serialization.seq",
        "core/bwtsa.seq",
        "core/empty.seq"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    PipelineTests, SeqTest,
    testing::Combine(
      testing::Values(
        "pipeline/parallel.seq",
        "pipeline/prefetch.seq",
        "pipeline/revcomp_opt.seq",
        "pipeline/canonical_opt.seq",
        "pipeline/interalign.seq"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    StdlibTests, SeqTest,
    testing::Combine(
      testing::Values(
        "stdlib/str_test.seq",
        "stdlib/math_test.seq",
        "stdlib/itertools_test.seq",
        "stdlib/bisect_test.seq",
        "stdlib/random_test.seq",
        "stdlib/statistics_test.seq",
        "stdlib/sort_test.seq",
        "stdlib/heapq_test.seq",
        "stdlib/operator_test.seq",
        "python/pybridge.seq"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    OptTests, SeqTest,
    testing::Combine(
        testing::Values(
            "transform/canonical.seq",
            "transform/dict_opt.seq",
            "transform/folding.seq",
            "transform/for_lowering.seq",
            "transform/io_opt.seq",
            "transform/inlining.seq",
            "transform/omp.seq",
            "transform/outlining.seq",
            "transform/str_opt.seq"
        ),
        testing::Values(true, false),
        testing::Values(""),
        testing::Values(""),
        testing::Values(0),
        testing::Values(false)
    ),
    getTestNameFromParam);

// clang-format on

int main(int argc, char *argv[]) {
  argv0 = ast::executable_path(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
