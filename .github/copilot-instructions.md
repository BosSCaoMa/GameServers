# GameServers Copilot Instructions

## Architecture & Key Modules
- `src/common/ParseHttp.{h,cpp}` implements the in-house HTTP parser used by every gateway/login handler. It lazily parses the body (JSON or form) only when a caller requests it and logs malformed requests through `LogM`.
- `src/Login/RecvProc.cpp` hosts the blocking TCP accept loop for the login microservice. Each accepted socket is handled on its own detached thread (`handle_client`) and routed by `HttpRequest::getPath()` (currently `/api/login` and `/api/register`). Keep-alive is expected, so do not eagerly `close(client_fd)` unless the request is invalid.
- `src/DataServer/DBConnPool.{h,cpp}` wraps MySQL Connector/C++ with a shared-pointer-based pool. Connections are created outside the mutex, validated via `SELECT 1`, and must be returned with `returnConnection` when work finishes.
- `lib/LogM.h` exposes the global logger and macros (`LOG_DEBUG`, `LOG_ERROR`, etc.). It truncates long messages; favor concise, pre-formatted strings.
- `lib/json.hpp` (nlohmann::json) is the only JSON dependency. `HttpRequest::getJson()` returns a cached reference, so keep the `HttpRequest` alive while you access it.

## HTTP Handling Patterns
- `HttpRequest` header values keep the leading space after `:` (e.g., `" Host" -> " example.com"`); trim manually if you need a clean value. Body helpers already return trimmed data.
- Body parsing is deferred until `getBodyParam`/`getJson` is called. Always call those helpers instead of manually decoding `body` so the lazy state stays coherent.
- When adding new endpoints, mirror the existing pattern: string-compare on `getPath()`, process, then either write to `client_fd` or schedule async work. Centralize socket cleanup inside `handle_client` error branches.
- Buffering uses a fixed 8 KB stack string. For larger payloads, extend the buffer size and ensure `read` loops until the full `Content-Length` is satisfied.

## Data & Persistence
- Acquire DB handles with `auto conn = pool.getConnection();` and guard them with `std::shared_ptr`. On early returns, call `pool.returnConnection(conn);` (consider a small RAII wrapper if you touch multiple exit points).
- The pool counter (`currentConnections_`) is manually managed; always decrement when you drop a broken connection to avoid exhausting the pool.
- Set connection DSN in the `DBConnPool` constructor (host/user/password/database). Credentials are currently hard-coded at call sites; plan for configuration injection when expanding services.

## Logging & Diagnostics
- Set global verbosity via `LogM::getInstance().setLevel(LogLevel::DEBUG);` before spinning up worker threads. Per the pain note in `todo.md`, be deliberate about releasing per-client resources and log both success and failure paths with the socket fd for traceability.
- Use the provided macros—`LOG_BASE` already stamps file/line/function and rotates files when needed; avoid custom printf logging.

## Build & Test Workflow
- Unit tests live under `testcode/` and are the fastest feedback loop. Build them with PowerShell:
  - `cmake -S testcode -B build/testcode`
  - `cmake --build build/testcode`
  - `ctest --test-dir build/testcode`
- `ParseHttp` is the only component under test today; extend its coverage before touching login parsing to avoid regressions.
- For ad-hoc server experiments, compile with your preferred compiler by including `src/common`, `src/DataServer`, `src/Login/include`, and `lib` in the include path, and link against MySQL Connector/C++ (`mysqlcppconn`) if you touch the DB layer.

## Conventions & Tips
- Stick to ASCII logs and protocol text; sockets currently assume UTF-8/ASCII and do not handle BOMs.
- Reuse the existing `HttpRequest` utility in new services; duplicating parsers will create divergence with the shared tests.
- When adding sockets or background threads, follow the detached-thread model already used in `ProcLoginReq`, or migrate all handlers to a thread pool at once—mixing models makes lifecycle bugs harder to debug.
- Document new endpoints in `README.md` next to the existing `/api/login` note so front-end teams know when payloads change.
