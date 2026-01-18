/**
 * @file script_executor.cpp
 * @brief Script execution utilities for running Python in IDA.
 */

#include <ida_chat/core/script_executor.hpp>

#include <ida_chat/common/warn_off.hpp>
#include <ida.hpp>
#include <kernwin.hpp>
#include <expr.hpp>
#include <idp.hpp>
#include <ida_chat/common/warn_on.hpp>

#include <sstream>
#include <chrono>

namespace ida_chat {

// ============================================================================
// Forward declarations for internal functions
// ============================================================================

namespace {

// Execute Python expression using IDA's extlang interface (for getting values)
bool run_python_expr(const char* code, qstring* output, qstring* errbuf);

// Execute Python statements using IDA's extlang interface (for assignments, loops, etc.)
bool run_python_statements(const char* code, qstring* errbuf);

} // anonymous namespace

// ============================================================================
// OutputCapture Implementation
// ============================================================================

struct OutputCapture::Impl {
    bool capturing = false;
    std::string captured_output;
    
    // Python code to capture stdout/stderr
    static constexpr const char* CAPTURE_START = R"(
import sys
from io import StringIO
__ida_chat_captured_stdout = sys.stdout
__ida_chat_captured_stderr = sys.stderr
__ida_chat_capture_buffer = StringIO()
sys.stdout = __ida_chat_capture_buffer
sys.stderr = __ida_chat_capture_buffer
)";

    static constexpr const char* CAPTURE_END = R"(
import sys
sys.stdout = __ida_chat_captured_stdout
sys.stderr = __ida_chat_captured_stderr
__ida_chat_output = __ida_chat_capture_buffer.getvalue()
del __ida_chat_captured_stdout
del __ida_chat_captured_stderr
del __ida_chat_capture_buffer
)";

    static constexpr const char* GET_OUTPUT = R"(
__ida_chat_output
)";
};

OutputCapture::OutputCapture() : impl_(std::make_unique<Impl>()) {}

OutputCapture::~OutputCapture() {
    if (impl_->capturing) {
        (void)stop();
    }
}

void OutputCapture::start() {
    if (impl_->capturing) return;
    
    // Execute Python statements to redirect stdout/stderr
    qstring errbuf;
    if (run_python_statements(Impl::CAPTURE_START, &errbuf)) {
        impl_->capturing = true;
        impl_->captured_output.clear();
    }
}

std::string OutputCapture::stop() {
    if (!impl_->capturing) return {};
    
    // Execute Python statements to restore stdout/stderr and get output
    qstring output, errbuf;
    run_python_statements(Impl::CAPTURE_END, &errbuf);
    impl_->capturing = false;
    
    // Get the captured output via a Python expression
    if (run_python_expr(Impl::GET_OUTPUT, &output, &errbuf)) {
        impl_->captured_output = output.c_str();
    }
    
    // Clean up the global variable
    run_python_statements("del __ida_chat_output", nullptr);
    
    return impl_->captured_output;
}

bool OutputCapture::is_capturing() const noexcept {
    return impl_->capturing;
}

// ============================================================================
// ScopedOutputCapture
// ============================================================================

ScopedOutputCapture::ScopedOutputCapture() {
    capture_.start();
}

ScopedOutputCapture::~ScopedOutputCapture() {
    if (capture_.is_capturing()) {
        (void)capture_.stop();
    }
}

std::string ScopedOutputCapture::get_output() {
    return capture_.stop();
}

// ============================================================================
// Script Execution Functions
// ============================================================================

namespace {

// Execute Python expression using IDA's extlang interface
// This is for evaluating expressions that return a value
bool run_python_expr(const char* code, qstring* output, qstring* errbuf) {
    // Find the Python extlang
    const extlang_t* python = find_extlang_by_name("Python");
    if (python == nullptr) {
        if (errbuf) {
            *errbuf = "Python extlang not found";
        }
        return false;
    }
    
    // Create IDC value for result
    idc_value_t result;
    
    // Compile and run the expression
    qstring local_errbuf;
    bool success = python->eval_expr(&result, BADADDR, code, errbuf ? errbuf : &local_errbuf);
    
    if (success && output) {
        // Convert result to string
        if (result.vtype == VT_STR) {
            *output = result.qstr();
        } else {
            qstring result_str;
            print_idcv(&result_str, result);
            *output = result_str;
        }
    }
    
    return success;
}

// Execute Python statements using IDA's extlang interface
// This is for executing code with statements (assignments, loops, etc.)
bool run_python_statements(const char* code, qstring* errbuf) {
    // Find the Python extlang
    const extlang_t* python = find_extlang_by_name("Python");
    if (python == nullptr) {
        if (errbuf) {
            *errbuf = "Python extlang not found";
        }
        return false;
    }
    
    // Use eval_snippet for statements (not eval_expr which is for expressions only)
    qstring local_errbuf;
    bool success = python->eval_snippet(code, errbuf ? errbuf : &local_errbuf);
    
    return success;
}

// Setup code to inject 'db' into the global namespace
// This creates the ida_domain Database object that scripts expect
// We store it in __builtins__ to ensure it persists across eval_snippet calls
static constexpr const char* DB_SETUP_CODE = R"PYTHON(
import sys
import builtins

# Debug: print Python path info
print(f"[IDA Chat] Python: {sys.executable}")
print(f"[IDA Chat] sys.path: {sys.path[:3]}...")

# Check if db already exists in builtins
if not hasattr(builtins, 'db') or builtins.db is None:
    try:
        from ida_domain import Database
        builtins.db = Database.open()
        print(f"[IDA Chat] Initialized db: {builtins.db.module}")
    except ImportError as e:
        # ida_domain not found - try to find where it might be
        print(f"[IDA Chat] ERROR: {e}")
        
        # Check if it's installed but not in path
        import subprocess
        result = subprocess.run([sys.executable, "-m", "pip", "show", "ida-domain"], 
                              capture_output=True, text=True)
        if result.returncode == 0:
            print(f"[IDA Chat] ida-domain IS installed but not in path:")
            print(result.stdout)
            # Try to find and add the location
            for line in result.stdout.split('\n'):
                if line.startswith('Location:'):
                    location = line.split(':', 1)[1].strip()
                    print(f"[IDA Chat] Adding {location} to sys.path")
                    sys.path.insert(0, location)
                    from ida_domain import Database
                    builtins.db = Database.open()
                    print(f"[IDA Chat] SUCCESS after path fix: {builtins.db.module}")
                    break
        else:
            print(f"[IDA Chat] ida-domain is NOT installed")
            print("[IDA Chat] Install: pip install ida-domain")
            raise
    except Exception as e:
        print(f"[IDA Chat] ERROR: Failed to open database: {e}")
        raise

# Make db available in global scope for this execution
db = builtins.db
)PYTHON";

// exec_request_t implementation for execute_sync
struct ScriptExecRequest : public exec_request_t {
    std::string code;
    ScriptResult result;
    
    explicit ScriptExecRequest(const std::string& c) : code(c) {}
    
    ssize_t idaapi execute() override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // First, ensure 'db' is available in the global namespace
        qstring setup_errbuf;
        if (!run_python_statements(DB_SETUP_CODE, &setup_errbuf)) {
            result.success = false;
            result.error = "Failed to initialize db: " + std::string(setup_errbuf.c_str());
            return 0;
        }
        
        // Capture output using our wrapper
        OutputCapture capture;
        capture.start();
        
        // Execute the Python statements (not expressions!)
        qstring errbuf;
        bool success = run_python_statements(code.c_str(), &errbuf);
        
        // Get captured output (stdout/stderr)
        std::string captured = capture.stop();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        result.success = success;
        // Use captured stdout/stderr output
        result.output = captured;
        if (!success && errbuf.length() > 0) {
            result.error = errbuf.c_str();
        }
        result.execution_time_ms = duration.count() / 1000.0;
        
        return 0;
    }
};

} // anonymous namespace

ScriptResult execute_script_direct(const std::string& code) {
    ScriptExecRequest req(code);
    req.execute();
    return req.result;
}

ScriptResult execute_script_on_main_thread(const std::string& code) {
    // Check if we're already on the main thread
    if (is_main_thread()) {
        return execute_script_direct(code);
    }
    
    ScriptExecRequest req(code);
    
    // Use execute_sync to run on main thread
    // MFF_WRITE allows modification of the database
    execute_sync(req, MFF_WRITE);
    
    return req.result;
}

ScriptExecutorFn create_main_thread_executor() {
    return [](const std::string& code) -> ScriptResult {
        return execute_script_on_main_thread(code);
    };
}

bool is_main_thread() {
    // IDA provides is_main_thread() function
    return ::is_main_thread();
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string format_script_error(
    const std::string& code,
    const std::string& error,
    int line_number) 
{
    std::ostringstream oss;
    oss << "Script execution error";
    if (line_number >= 0) {
        oss << " at line " << line_number;
    }
    oss << ":\n" << error;
    
    // Add code context if available
    if (!code.empty()) {
        auto lines = split(code, '\n');
        int start_line = (line_number >= 0) ? std::max(0, line_number - 2) : 0;
        int end_line = (line_number >= 0) ? std::min(static_cast<int>(lines.size()), line_number + 3) : static_cast<int>(lines.size());
        
        oss << "\n\nCode context:\n";
        for (int i = start_line; i < end_line; ++i) {
            oss << (i + 1) << ": ";
            if (i == line_number - 1) {
                oss << ">>> " << lines[i] << " <<<";
            } else {
                oss << "    " << lines[i];
            }
            oss << "\n";
        }
    }
    
    return oss.str();
}

std::string validate_script_syntax(const std::string& code) {
    // Use Python's ast module to check syntax without executing
    std::string escaped_code = replace_all(code, "'''", "\\'\\'\\'");
    
    // Step 1: Run the validation statements
    std::string check_stmts = R"(
import ast
try:
    ast.parse(r''')" + escaped_code + R"(''')
    __ida_chat_result = ""
except SyntaxError as e:
    __ida_chat_result = f"{e.msg} at line {e.lineno}"
)";

    qstring errbuf;
    if (!run_python_statements(check_stmts.c_str(), &errbuf)) {
        return "Failed to validate syntax: " + std::string(errbuf.c_str());
    }
    
    // Step 2: Get the result via expression evaluation
    qstring output;
    if (!run_python_expr("__ida_chat_result", &output, &errbuf)) {
        return "Failed to get validation result";
    }
    
    // The output is the result of the validation
    std::string result = output.c_str();
    
    // Clean up
    run_python_statements("del __ida_chat_result", nullptr);
    
    // Strip quotes if present
    if (result.size() >= 2 && result.front() == '\'' && result.back() == '\'') {
        result = result.substr(1, result.size() - 2);
    }
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.size() - 2);
    }
    
    return result; // Empty string means valid
}

} // namespace ida_chat
