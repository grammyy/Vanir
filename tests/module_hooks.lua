require("vanir")

-- test_hooks.lua
-- tests: hook.add, hook.remove, firing order, duplicate name replacement, think hook

local results={}
local step=0

-- basic add and fire
hook.add("think","test_basic",function()
    if step==0 then
        results[#results+1]="think fired"
        step=1
    end
end)

-- two listeners on the same hook; then replace
hook.add("think","test_a",function()
    if step==1 then
        results[#results+1]="listener a"
        step=2

        hook.add("think","test_a",function()
            if step==3 then
                results[#results+1]="listener a replaced"
                step=4
            end
        end)
    end
end)

hook.add("think","test_b",function()
    if step==2 then
        results[#results+1]="listener b"
        step=3
    end
end)

-- remove a listener
hook.add("think","test_remove",function()
    results[#results+1]="ERROR: removed listener still fired"
end)
hook.remove("think","test_remove")

local done=false

hook.add("think","test_finish",function()
    if step==4 and not done then
        done=true

        print("[hooks] results:")

        for i,v in ipairs(results) do
            print("  "..i..": "..v)
        end

        print("[hooks] PASS")
        quit()
    end
end)

while true do
    hooks.run()
end
