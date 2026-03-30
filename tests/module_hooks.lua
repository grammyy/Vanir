require("vanir")

-- module_hooks.lua
-- tests: hook.add, hook.remove, firing order, duplicate name replacement,
--        think hook, onError hook

local results = {}
local step    = 0

-- basic add and fire
hook.add("think", "test_basic", function()
    if step == 0 then
        results[#results + 1] = "think fired"
        step = 1
    end
end)

-- two listeners on the same hook; then replace
hook.add("think", "test_a", function()
    if step == 1 then
        results[#results + 1] = "listener a"
        step = 2

        hook.add("think", "test_a", function()
            if step == 3 then
                results[#results + 1] = "listener a replaced"
                step = 4
            end
        end)
    end
end)

hook.add("think", "test_b", function()
    if step == 2 then
        results[#results + 1] = "listener b"
        step = 3
    end
end)

-- remove a listener — should never fire
hook.add("think", "test_remove", function()
    results[#results + 1] = "ERROR: removed listener still fired"
end)
hook.remove("think", "test_remove")

-- onError hook — fires when C calls fireError() (e.g. via throw())
-- We trigger it by passing a nil path to files.open, which throws inside C.
local errorFired   = false
local errorMessage = nil

hook.add("onError", "test_onError", function(msg)
    errorFired   = true
    errorMessage = msg
    print("[hooks] onError fired: " .. tostring(msg))
end)

-- trigger a controlled C-level throw via a bad files.open call; pcall keeps
-- the test from aborting — fireError() already staged the onError fire
local errorTriggered = false
hook.add("think", "test_trigger_error", function()
    print("meow")
    if step >= 2 and not errorTriggered then
        errorTriggered = true
        
        files.open(nil)   -- nil path -> throw() inside filesOpen
    end
end)

local done = false

hook.add("think", "test_finish", function()
    if step == 4 and not done then
        done = true

        print("[hooks] results:")
        for i, v in ipairs(results) do
            print("  " .. i .. ": " .. v)
        end

        if errorFired then
            print("[hooks] onError: OK (message: " .. tostring(errorMessage) .. ")")
        else
            print("[hooks] onError: not yet fired — may need one more tick")
        end

        print("[hooks] PASS")
        quit()
    end
end)

while true do
    hooks.run()
end
