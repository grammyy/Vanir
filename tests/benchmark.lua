-- ============================================================
--  benchmark.lua  –  Pure Lua Performance Suite
--  Usage:  luajit benchmark.lua   /   lua54 benchmark.lua
-- ============================================================

local ITERATIONS = 1e7   -- 10 million (reduce if too slow)

-- ── Utilities ────────────────────────────────────────────────
local clock = os.clock

local function bench(name, fn)
  collectgarbage("collect")
  collectgarbage("stop")
  local t0 = clock()
  fn()
  local t1 = clock()
  collectgarbage("restart")
  local ms = (t1 - t0) * 1000
  print(string.format("  %-35s %8.2f ms", name, ms))
  return ms
end

local function section(title)
  print("")
  print(string.rep("-", 55))
  print("  " .. title)
  print(string.rep("-", 55))
end

local results = {}
local function record(category, name, fn)
  local ms = bench(name, fn)
  results[#results+1] = { cat = category, name = name, ms = ms }
end

-- ── Header ───────────────────────────────────────────────────
print("")
print("  ╔══════════════════════════════════════════════════╗")
print("  ║         Pure Lua Benchmark Suite                 ║")
print("  ║  iterations per test : " .. string.format("%-10s", string.format("%.0e", ITERATIONS)) .. "                 ║")
print("  ╚══════════════════════════════════════════════════╝")

-- ════════════════════════════════════════════════════════════
--  1. LOOPS & ITERATION
-- ════════════════════════════════════════════════════════════
section("1. LOOPS & ITERATION")

record("loops", "numeric for (count up)", function()
  local s = 0
  for i = 1, ITERATIONS do s = s + i end
  return s
end)

record("loops", "numeric for (count down)", function()
  local s = 0
  for i = ITERATIONS, 1, -1 do s = s + i end
  return s
end)

record("loops", "while loop", function()
  local s, i = 0, 0
  while i < ITERATIONS do i = i + 1; s = s + i end
  return s
end)

record("loops", "repeat-until loop", function()
  local s, i = 0, 0
  repeat i = i + 1; s = s + i until i >= ITERATIONS
  return s
end)

record("loops", "ipairs over table (1M)", function()
  local N = ITERATIONS / 10
  local t = {}
  for i = 1, N do t[i] = i end
  local s = 0
  for _, v in ipairs(t) do s = s + v end
  return s
end)

record("loops", "pairs over table (1M)", function()
  local N = ITERATIONS / 10
  local t = {}
  for i = 1, N do t[i] = i end
  local s = 0
  for k, v in pairs(t) do s = s + v end
  return s
end)

-- ════════════════════════════════════════════════════════════
--  2. MATH / NUMBER CRUNCHING
-- ════════════════════════════════════════════════════════════
section("2. MATH / NUMBER CRUNCHING")

record("math", "integer addition", function()
  local s = 0
  for i = 1, ITERATIONS do s = s + i end
  return s
end)

record("math", "float multiply", function()
  local s = 1.0
  for i = 1, ITERATIONS do s = s * 1.0000001 end
  return s
end)

record("math", "math.sqrt", function()
  local s = 0
  for i = 1, ITERATIONS do s = math.sqrt(i) end
  return s
end)

record("math", "math.sin", function()
  local s = 0
  for i = 1, ITERATIONS do s = math.sin(i) end
  return s
end)

record("math", "math.floor", function()
  local s = 0
  for i = 1, ITERATIONS do s = math.floor(i * 1.5) end
  return s
end)

record("math", "modulo (%)", function()
  local s = 0
  for i = 1, ITERATIONS do s = i % 7 end
  return s
end)

record("math", "bitwise XOR (10M)", function()
  local s = 0
  local bit_xor
  -- LuaJIT uses bit library, Lua 5.3+ uses ~ operator
  if type(bit) == "table" and bit.bxor then
    bit_xor = bit.bxor
    for i = 1, ITERATIONS do s = bit_xor(s, i) end
  else
    local ok = pcall(function()
      local fn = load("local s=0; for i=1,"..ITERATIONS.." do s=s~i end return s")
      s = fn()
    end)
    if not ok then
      for i = 1, ITERATIONS do s = (s + i) % 256 end
    end
  end
  return s
end)

-- ════════════════════════════════════════════════════════════
--  3. STRING OPERATIONS
-- ════════════════════════════════════════════════════════════
section("3. STRING OPERATIONS")

local N_STR = ITERATIONS / 100   -- strings are expensive, use 100k

record("string", "string.format (100k)", function()
  local s
  for i = 1, N_STR do s = string.format("value=%d", i) end
  return s
end)

record("string", "string.len (100k)", function()
  local str = string.rep("x", 100)
  local s = 0
  for i = 1, N_STR do s = string.len(str) end
  return s
end)

record("string", "string.sub (100k)", function()
  local str = string.rep("abcde", 20)
  local s
  for i = 1, N_STR do s = string.sub(str, 2, 10) end
  return s
end)

record("string", "string.upper (100k)", function()
  local str = "hello world benchmark"
  local s
  for i = 1, N_STR do s = string.upper(str) end
  return s
end)

record("string", "string.find (100k)", function()
  local str = "the quick brown fox jumps"
  local s
  for i = 1, N_STR do s = string.find(str, "fox") end
  return s
end)

record("string", "string.gsub (100k)", function()
  local str = "hello world"
  local s
  for i = 1, N_STR do s = string.gsub(str, "world", "lua") end
  return s
end)

record("string", ".. concatenation (100k)", function()
  local s = ""
  for i = 1, N_STR do s = "prefix_" .. i end
  return s
end)

record("string", "table.concat builder (100k)", function()
  local t = {}
  for i = 1, N_STR do t[i] = tostring(i) end
  return table.concat(t, ",")
end)

-- ════════════════════════════════════════════════════════════
--  4. TABLE OPERATIONS
-- ════════════════════════════════════════════════════════════
section("4. TABLE OPERATIONS")

local N_TBL = ITERATIONS / 10   -- 1M

record("table", "array write (1M)", function()
  local t = {}
  for i = 1, N_TBL do t[i] = i end
  return t
end)

record("table", "array read (1M)", function()
  local t = {}
  for i = 1, N_TBL do t[i] = i end
  local s = 0
  for i = 1, N_TBL do s = s + t[i] end
  return s
end)

record("table", "hash write (1M)", function()
  local t = {}
  for i = 1, N_TBL do t["k"..i] = i end
  return t
end)

record("table", "hash read (1M)", function()
  local t = {}
  for i = 1, N_TBL do t["k"..i] = i end
  local s = 0
  for i = 1, N_TBL do s = s + t["k"..i] end
  return s
end)

record("table", "table.insert (1M)", function()
  local t = {}
  for i = 1, N_TBL do table.insert(t, i) end
  return t
end)

record("table", "table.remove from end (1M)", function()
  local t = {}
  for i = 1, N_TBL do t[i] = i end
  for i = 1, N_TBL do table.remove(t) end
  return t
end)

record("table", "nested table access (1M)", function()
  local t = { a = { b = { c = 42 } } }
  local s = 0
  for i = 1, N_TBL do s = t.a.b.c end
  return s
end)

-- ════════════════════════════════════════════════════════════
--  5. FUNCTION CALL OVERHEAD
-- ════════════════════════════════════════════════════════════
section("5. FUNCTION CALL OVERHEAD")

local function noop() end
local function add(a, b) return a + b end
local function fib(n)
  if n < 2 then return n end
  return fib(n-1) + fib(n-2)
end

record("calls", "call empty function (10M)", function()
  for i = 1, ITERATIONS do noop() end
end)

record("calls", "call with args + return (10M)", function()
  local s = 0
  for i = 1, ITERATIONS do s = add(i, i) end
  return s
end)

record("calls", "local function call (10M)", function()
  local function localfn(x) return x * 2 end
  local s = 0
  for i = 1, ITERATIONS do s = localfn(i) end
  return s
end)

record("calls", "closure capture (10M)", function()
  local base = 100
  local function closure(x) return x + base end
  local s = 0
  for i = 1, ITERATIONS do s = closure(i) end
  return s
end)

record("calls", "method call via : (10M)", function()
  local obj = { val = 0 }
  function obj:inc(x) self.val = self.val + x end
  for i = 1, ITERATIONS do obj:inc(1) end
  return obj.val
end)

record("calls", "recursive fib(30) x100", function()
  local s = 0
  for i = 1, 100 do s = fib(30) end
  return s
end)

record("calls", "vararg function (10M)", function()
  local function varfn(...) local t = {...}; return t[1] end
  local s = 0
  for i = 1, ITERATIONS do s = varfn(i, i+1, i+2) end
  return s
end)

-- ════════════════════════════════════════════════════════════
--  SUMMARY
-- ════════════════════════════════════════════════════════════
print("")
print(string.rep("=", 55))
print("  SUMMARY  (sorted fastest → slowest per category)")
print(string.rep("=", 55))

-- group by category
local cats = {}
local seen = {}
for _, r in ipairs(results) do
  if not seen[r.cat] then
    seen[r.cat] = true
    cats[#cats+1] = r.cat
  end
end

local total = 0
for _, cat in ipairs(cats) do
  local group = {}
  for _, r in ipairs(results) do
    if r.cat == cat then group[#group+1] = r end
  end
  table.sort(group, function(a,b) return a.ms < b.ms end)
  print("")
  print("  [" .. cat:upper() .. "]")
  for _, r in ipairs(group) do
    total = total + r.ms
    print(string.format("    %-33s %8.2f ms", r.name, r.ms))
  end
end

print("")
print(string.rep("-", 55))
print(string.format("  %-35s %8.2f ms", "TOTAL", total))
print(string.rep("=", 55))
print("")
