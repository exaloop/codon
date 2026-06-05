// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <gc.h>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
#include <io.h>
// No fork()/wait() on Windows. runInChildProcess() re-execs the test binary once
// per case, so the child's exit code maps directly onto these wait-status macros.
// (<windows.h> itself is included below, after the LLVM/Codon headers, so its
// macros can't clobber LLVM identifiers like min/max or GDI's PASSTHROUGH.)
#define WIFEXITED(s) (true)
#define WEXITSTATUS(s) (s)
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

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

#ifdef _WIN32
// Pull in the Win32 API only after the LLVM/Codon headers so windows.h's macros
// (min/max, GDI's PASSTHROUGH, ...) can't clobber LLVM's identifiers.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#include <windows.h>
#endif

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

  while (getline(stream, line, delim)) {
    // Windows text-mode stdout writes \r\n; drop the trailing \r so captured
    // output compares equal to the (LF) expectations.
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    result.push_back(line);
  }

  return result;
}

static pair<bool, string> findExpectOnLine(const string &rawLine) {
  // Drop a trailing \r so CRLF-checked-out test files match LF program output.
  string line = rawLine;
  if (!line.empty() && line.back() == '\r')
    line.pop_back();
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
void seq_exc_init(int flags);

// Compile (and optionally run) a single test case. Extracted so the same logic
// serves both the POSIX forked child and the Windows re-exec'd child process.
static void runCompileAndRun(const string &file, bool debug, const string &code,
                             int startLine, int testFlags, bool pyNumerics,
                             bool run) {
  auto options = Options::getDefault(argv0);
  options->test = true;
  options->standalone = true;
  options->debug = debug;
  options->pynum = pyNumerics;

  auto compiler = std::make_unique<Compiler>(*options);
  // make sure we abort() on runtime error
  llvm::handleAllErrors(code.empty()
                            ? compiler->parseFile(file, testFlags)
                            : compiler->parseCode(file, code, startLine, testFlags),
                        [](const error::ParserErrorInfo &e) {
                          for (auto &group : e.getErrors()) {
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

  if (run)
    compiler->getLLVMVisitor()->run({file});
  fflush(stdout);
}

class SeqTest
    : public testing::TestWithParam<tuple<
          string /*filename*/, bool /*debug*/, string /* case name */,
          string /* case code */, int /* case line */, bool /* barebones stdlib */,
          bool /* Python numerics */, bool /* run */>> {
  vector<char> buf;
#ifndef _WIN32
  int out_pipe[2];
  pid_t pid;
#endif

public:
#ifdef _WIN32
  SeqTest() : buf(65536) {}
#else
  SeqTest() : buf(65536), out_pipe(), pid() {}
#endif
  string getFilename(const string &basename) {
    return string(TEST_DIR) + "/" + basename;
  }
  int runInChildProcess(bool avoidFork = false) {
    (void)avoidFork;
    auto file = getFilename(get<0>(GetParam()));
    bool debug = get<1>(GetParam());
    auto code = get<3>(GetParam());
    int startLine = get<4>(GetParam());
    int testFlags = 1 + get<5>(GetParam());
    bool pyNumerics = get<6>(GetParam());
    bool run = get<7>(GetParam());

#ifdef _WIN32
    // No fork() on Windows: serialize this case's parameters to a temp file, then
    // re-exec the test binary in "--run-case" mode with its stdout wired to a pipe.
    // A fresh process per case mirrors the isolation fork() gave us (the JIT/runtime
    // is set up once per process) and contains any abort() inside the child.
    char tmpDir[MAX_PATH], tmpFile[MAX_PATH];
    GetTempPathA(MAX_PATH, tmpDir);
    GetTempFileNameA(tmpDir, "cdt", 0, tmpFile);
    {
      std::ofstream pf(tmpFile, std::ios::binary);
      pf << debug << ' ' << startLine << ' ' << testFlags << ' ' << pyNumerics << ' '
         << run << '\n';
      auto writeStr = [&](const string &s) {
        pf << s.size() << '\n';
        pf.write(s.data(), s.size());
        pf << '\n';
      };
      writeStr(file);
      writeStr(code);
    }

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    assert(CreatePipe(&rd, &wr, &sa, 0));
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    string cmd = "\"" + argv0 + "\" --run-case \"" + string(tmpFile) + "\"";
    vector<char> cmdv(cmd.begin(), cmd.end());
    cmdv.push_back('\0');

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = wr;
    si.hStdError = wr;
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, cmdv.data(), nullptr, nullptr, TRUE, 0, nullptr,
                             nullptr, &si, &pi);
    CloseHandle(wr);
    assert(ok);

    string out;
    char rbuf[4096];
    DWORD n = 0;
    while (ReadFile(rd, rbuf, sizeof(rbuf), &n, nullptr) && n > 0)
      out.append(rbuf, n);
    CloseHandle(rd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD ec = 0;
    GetExitCodeProcess(pi.hProcess, &ec);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    DeleteFileA(tmpFile);

    std::fill(buf.begin(), buf.end(), '\0');
    memcpy(buf.data(), out.data(), std::min(out.size(), buf.size() - 1));
    return static_cast<int>(ec);
#else
    auto fn = [&]() {
      runCompileAndRun(file, debug, code, startLine, testFlags, pyNumerics, run);
    };

    assert(pipe(out_pipe) != -1);
    pid = fork();
    GC_atfork_prepare();
    assert(pid != -1);

    if (pid == 0) {
      GC_atfork_child();
      dup2(out_pipe[1], STDOUT_FILENO);
      close(out_pipe[0]);
      close(out_pipe[1]);
      fn();
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
#endif
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
  if (!WIFEXITED(status))
    std::cerr << result() << std::endl;
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
  vector<tuple<string, bool, string, string, int, bool, bool, bool>> cases;
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
        if (line && testName != "__ignore__") {
          cases.emplace_back(make_tuple(f, true, to_string(line) + "_" + testName, code,
                                        codeLine, barebones, false, true));
        }
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
    if (line && testName != "__ignore__") {
      cases.emplace_back(make_tuple(f, true, to_string(line) + "_" + testName, code,
                                    codeLine, barebones, false, true));
    }
  }
  return cases;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    TypeTests, SeqTest,
    testing::ValuesIn(getTypeTests({
      "parser/typecheck/test_access.codon",
      "parser/typecheck/test_assign.codon",
      "parser/typecheck/test_basic.codon",
      "parser/typecheck/test_call.codon",
      "parser/typecheck/test_class.codon",
      "parser/typecheck/test_collections.codon",
      "parser/typecheck/test_cond.codon",
      "parser/typecheck/test_ctx.codon",
      "parser/typecheck/test_error.codon",
      "parser/typecheck/test_function.codon",
      "parser/typecheck/test_import.codon",
      "parser/typecheck/test_infer.codon",
      "parser/typecheck/test_loops.codon",
      "parser/typecheck/test_op.codon",
      "parser/typecheck/test_parser.codon",
      "parser/typecheck/test_python.codon",
      "parser/typecheck/test_typecheck.codon"
    })),
    getTypeTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    CoreTests, SeqTest,
    testing::Combine(
      testing::Values(
        "core/helloworld.codon",
        "core/arithmetic.codon",
        "core/numerics.codon",
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
        "core/empty.codon",
        "core/vec_simd.codon"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false),
      testing::Values(true),
      testing::Values(true)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    StdlibTests, SeqTest,
    testing::Combine(
      testing::Values(
        "stdlib/llvm_test.codon",
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
        "stdlib/asyncio_test.codon",
        "python/pybridge.codon"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false),
      testing::Values(true),
      testing::Values(true)
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    CNumericsTests, SeqTest,
    testing::Combine(
      testing::Values(
        "core/numerics.codon",
        "stdlib/math_test.codon"
      ),
      testing::Values(true, false),
      testing::Values(""),
      testing::Values(""),
      testing::Values(0),
      testing::Values(false),
      testing::Values(false),
      testing::Values(true)
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
        testing::Values(true),
        testing::Values(true)
    ),
    getTestNameFromParam);

// GPU (CODON_GPU=OFF) and numpy (carved out: no native Fortran/BLAS on MSVC) are
// unsupported on Windows — skip their suites so the runner reflects what's built.
#ifndef _WIN32
  INSTANTIATE_TEST_SUITE_P(
    GpuTests, SeqTest,
    testing::Combine(
        testing::Values(
            "transform/kernels.codon"
        ),
        testing::Values(true, false),
        testing::Values(""),
        testing::Values(""),
        testing::Values(0),
        testing::Values(false),
        testing::Values(true),
        testing::Values(false)  // do not run by default, just compile
    ),
    getTestNameFromParam);

INSTANTIATE_TEST_SUITE_P(
    NumPyTests, SeqTest,
    testing::Combine(
        testing::Values(
            "numpy/random_tests/test_mt19937.codon",
            "numpy/random_tests/test_pcg64.codon",
            "numpy/random_tests/test_philox.codon",
            "numpy/random_tests/test_sfc64.codon",
            "numpy/test_dtype.codon",
            "numpy/test_elision.codon",
            "numpy/test_fft.codon",
            "numpy/test_functional.codon",
            // "numpy/test_fusion.codon", // TODO: uses a lot of RAM
            "numpy/test_indexing.codon",
            "numpy/test_io.codon",
            "numpy/test_lib.codon",
            "numpy/test_linalg.codon",
            "numpy/test_loops.codon",
            // "numpy/test_misc.codon", // TODO: takes forever in debug mode
            "numpy/test_ndmath.codon",
            "numpy/test_npdatetime.codon",
            "numpy/test_pybridge.codon",
            "numpy/test_reductions.codon",
            "numpy/test_routines.codon",
            "numpy/test_sorting.codon",
            "numpy/test_statistics.codon",
            "numpy/test_ufunc.codon",
            "numpy/test_window.codon"
        ),
        testing::Values(true, false),
        testing::Values(""),
        testing::Values(""),
        testing::Values(0),
        testing::Values(false),
        testing::Values(true),
        testing::Values(true)
    ),
    getTestNameFromParam);
#endif // !_WIN32

// clang-format on

int main(int argc, char *argv[]) {
  argv0 = ast::Filesystem::executable_path(argv[0]).string();
#ifdef _WIN32
  // Child mode: a single case re-exec'd by runInChildProcess(). Read the
  // serialized parameters, compile/run the case, and let stdout + the exit code
  // flow back to the parent over the inherited pipe.
  if (argc >= 3 && string(argv[1]) == "--run-case") {
    std::ifstream pf(argv[2], std::ios::binary);
    bool debug = false, pyNumerics = false, run = false;
    int startLine = 0, testFlags = 0;
    pf >> debug >> startLine >> testFlags >> pyNumerics >> run;
    pf.get(); // consume newline
    auto readStr = [&]() {
      size_t len = 0;
      pf >> len;
      pf.get(); // consume newline
      string s(len, '\0');
      pf.read(&s[0], len);
      pf.get(); // consume trailing newline
      return s;
    };
    string file = readStr();
    string code = readStr();
    runCompileAndRun(file, debug, code, startLine, testFlags, pyNumerics, run);
    fflush(stdout);
    return EXIT_SUCCESS;
  }
#endif
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
