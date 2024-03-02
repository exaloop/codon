// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <cstdio>
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

#include "codon/cir/analyze/dataflow/capture.h"
#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/util/inlining.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/operator.h"
#include "codon/cir/util/outlining.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/error.h"
#include "codon/parser/common.h"
#include "codon/util/common.h"

#include "gtest/gtest.h"

using namespace codon;
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

struct PartitionArgsByEscape : public ir::util::Operator {
  std::vector<ir::analyze::dataflow::CaptureInfo> expected;
  std::vector<ir::Value *> calls;

  void handle(ir::CallInstr *v) override {
    using namespace codon::ir;
    if (auto *f = cast<Func>(util::getFunc(v->getCallee()))) {
      if (f->getUnmangledName() == "expect_capture") {
        // Format is:
        //   - Return captures (bool)
        //   - Extern captures (bool)
        //   - Captured arg indices (int tuple)
        std::vector<Value *> args(v->begin(), v->end());
        seqassertn(args.size() == 3, "bad escape-test call (size)");
        seqassertn(isA<BoolConst>(args[0]) && isA<BoolConst>(args[1]),
                   "bad escape-test call (arg types)");

        ir::analyze::dataflow::CaptureInfo info;
        info.returnCaptures = cast<BoolConst>(args[0])->getVal();
        info.externCaptures = cast<BoolConst>(args[1])->getVal();
        auto *tuple = cast<CallInstr>(args[2]);
        seqassertn(tuple,
                   "last escape-test call argument should be a const tuple literal");

        for (auto *arg : *tuple) {
          seqassertn(isA<IntConst>(arg), "final args should be int");
          info.argCaptures.push_back(cast<IntConst>(arg)->getVal());
        }

        expected.push_back(info);
        calls.push_back(v);
      }
    }
  }
};

struct EscapeValidator : public ir::transform::Pass {
  const std::string KEY = "test-escape-validator-pass";
  std::string getKey() const override { return KEY; }

  std::string capAnalysisKey;

  explicit EscapeValidator(const std::string &capAnalysisKey)
      : ir::transform::Pass(), capAnalysisKey(capAnalysisKey) {}

  void run(ir::Module *m) override {
    using namespace codon::ir;
    auto *capResult =
        getAnalysisResult<ir::analyze::dataflow::CaptureResult>(capAnalysisKey);
    for (auto *var : *m) {
      if (auto *f = cast<Func>(var)) {
        PartitionArgsByEscape pabe;
        f->accept(pabe);
        auto expected = pabe.expected;
        if (expected.empty())
          continue;

        auto it = capResult->results.find(f->getId());
        seqassertn(it != capResult->results.end(),
                   "function not found in capture results");
        auto received = it->second;
        seqassertn(expected.size() == received.size(),
                   "size mismatch in capture results");

        for (unsigned i = 0; i < expected.size(); i++) {
          auto exp = expected[i];
          auto got = received[i];
          std::sort(exp.argCaptures.begin(), exp.argCaptures.end());
          std::sort(got.argCaptures.begin(), got.argCaptures.end());

          bool good = (exp.returnCaptures == got.returnCaptures) &&
                      (exp.externCaptures == got.externCaptures) &&
                      (exp.argCaptures == got.argCaptures);
          pabe.calls[i]->replaceAll(m->getBool(good));
        }
      }
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

class SeqTest : public testing::TestWithParam<
                    tuple<string /*filename*/, bool /*debug*/, string /* case name */,
                          string /* case code */, int /* case line */,
                          bool /* barebones stdlib */, bool /* Python numerics */>> {
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
      bool debug = get<1>(GetParam());
      auto code = get<3>(GetParam());
      auto startLine = get<4>(GetParam());
      int testFlags = 1 + get<5>(GetParam());
      bool pyNumerics = get<6>(GetParam());

      auto compiler = std::make_unique<Compiler>(
          argv0, debug, /*disabledPasses=*/std::vector<std::string>{}, /*isTest=*/true,
          pyNumerics);
      compiler->getLLVMVisitor()->setStandalone(
          true); // make sure we abort() on runtime error
      llvm::handleAllErrors(code.empty()
                                ? compiler->parseFile(file, testFlags)
                                : compiler->parseCode(file, code, startLine, testFlags),
                            [](const error::ParserErrorInfo &e) {
                              for (auto &group : e) {
                                for (auto &msg : group) {
                                  getLogger().level = 0;
                                  printf("%s\n", msg.getMessage().c_str());
                                }
                              }
                              fflush(stdout);
                              exit(EXIT_FAILURE);
                            });

      auto *pm = compiler->getPassManager();
      pm->registerPass(std::make_unique<TestOutliner>());
      pm->registerPass(std::make_unique<TestInliner>());
      auto capKey =
          pm->registerAnalysis(std::make_unique<ir::analyze::dataflow::CaptureAnalysis>(
                                   ir::analyze::dataflow::RDAnalysis::KEY,
                                   ir::analyze::dataflow::DominatorAnalysis::KEY),
                               {ir::analyze::dataflow::RDAnalysis::KEY,
                                ir::analyze::dataflow::DominatorAnalysis::KEY});
      pm->registerPass(std::make_unique<EscapeValidator>(capKey), /*insertBefore=*/"",
                       {capKey});

      llvm::cantFail(compiler->compile());
      compiler->getLLVMVisitor()->run({file});
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
        EXPECT_EQ(results[i], expects.first[i]);
      else
        EXPECT_EQ(results[i], expects.first[i]);
    EXPECT_EQ(results.size(), expects.first.size());
  }
}
auto getTypeTests(const vector<string> &files) {
  vector<tuple<string, bool, string, string, int, bool, bool>> cases;
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
                                        codeLine, barebones, false));
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
                                    codeLine, barebones, false));
  }
  return cases;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    TypeTests, SeqTest,
    testing::ValuesIn(getTypeTests({
      "parser/simplify_expr.codon",
      "parser/simplify_stmt.codon",
      "parser/typecheck_expr.codon",
      "parser/typecheck_stmt.codon",
      "parser/types.codon",
      "parser/llvm.codon"
    })),
    getTypeTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    CoreTests, SeqTest,
    testing::Combine(
      testing::Values(
        "core/helloworld.codon",
        "core/arithmetic.codon",
        "core/parser.codon",
        "core/generics.codon",
        "core/generators.codon",
        "core/exceptions.codon",
        "core/containers.codon",
        "core/trees.codon",
        "core/range.codon",
        "core/bltin.codon",
        "core/arguments.codon",
        "core/match.codon",
        "core/serialization.codon",
        "core/pipeline.codon",
        "core/empty.codon"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false),
      testing::Values(false)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    NumericsTests, SeqTest,
    testing::Combine(
      testing::Values(
        "core/numerics.codon"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false),
      testing::Values(true)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    StdlibTests, SeqTest,
    testing::Combine(
      testing::Values(
        "stdlib/str_test.codon",
        "stdlib/re_test.codon",
        "stdlib/math_test.codon",
        "stdlib/cmath_test.codon",
        "stdlib/datetime_test.codon",
        "stdlib/itertools_test.codon",
        "stdlib/bisect_test.codon",
        "stdlib/random_test.codon",
        "stdlib/statistics_test.codon",
        "stdlib/sort_test.codon",
        "stdlib/heapq_test.codon",
        "stdlib/operator_test.codon",
        "python/pybridge.codon"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false),
      testing::Values(false)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    OptTests, SeqTest,
    testing::Combine(
        testing::Values(
            "transform/canonical.codon",
            "transform/dict_opt.codon",
            "transform/escapes.codon",
            "transform/folding.codon",
            "transform/for_lowering.codon",
            "transform/io_opt.codon",
            "transform/inlining.codon",
            "transform/list_opt.codon",
            "transform/omp.codon",
            "transform/outlining.codon",
            "transform/str_opt.codon"
        ),
        testing::Values(true, false),
        testing::Values(""),
        testing::Values(""),
        testing::Values(0),
        testing::Values(false),
        testing::Values(false)
    ),
    getTestNameFromParam);

// clang-format on

int main(int argc, char *argv[]) {
  argv0 = ast::executable_path(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
