print("Preload loading...\n")

local middleclass = {}

local function _createIndexWrapper(aClass, f)
    if f == nil then
        return aClass.__instanceDict
    end

    return function(self, name)
        local value = aClass.__instanceDict[name]

        if value ~= nil then
            return value
        elseif type(f) == "function" then
            return f(self, name)
        elseif type(f) == "table" then
            return f[name]
        end
    end
end
  
local function _propagateInstanceMethod(aClass, name, f)
    f = name == "__index" and _createIndexWrapper(aClass, f) or f
    aClass.__instanceDict[name] = f

    for subclass in pairs(aClass.subclasses) do
        if not rawget(subclass.__declaredMethods, name) then
            _propagateInstanceMethod(subclass, name, f)
        end
    end
end

local function _declareInstanceMethod(aClass, name, f)
    aClass.__declaredMethods[name] = f

    if not f and aClass.super then
        f = aClass.super.__instanceDict[name]
    end

    _propagateInstanceMethod(aClass, name, f)
end


local function _tostring(self)
    return "class " .. self.name
end
  
local function _call(self, ...)
    return self:new(...)
end  

local function _createClass(name, super)
    local dict = {}
    dict.__index = dict

    local aClass = {
        name = name,
        super = super,
        static = {},
        __instanceDict = dict,
        __declaredMethods = {},
        subclasses = setmetatable({}, {__mode = 'k'})
    }

    setmetatable(aClass.static, {
        __index = function(_, k)
            return rawget(dict, k) or (super and super.static[k])
        end
    })

    setmetatable(aClass, {
        __index = aClass.static,
        __tostring = _tostring,
        __call = _call,
        __newindex = _declareInstanceMethod
    })

    return aClass
end

local function _includeMixin(aClass, mixin)
    assert(type(mixin) == 'table', "mixin must be a table")

    for name, method in pairs(mixin) do
        if name ~= "included" and name ~= "static" then
        aClass[name] = method
        end
    end

    for name, method in pairs(mixin.static or {}) do
        aClass.static[name] = method
    end

    if type(mixin.included) == "function" then
        mixin:included(aClass)
    end

    return aClass
end  

local DefaultMixin = {
    __tostring = function(self)
        return "instance of " .. tostring(self.class)
    end,

    initialize = function(self, ...)
    end,

    isInstanceOf = function(self, aClass)
        return type(aClass) == 'table' and type(self) == 'table' and (
        self.class == aClass or (
            type(self.class) == 'table' and type(self.class.isSubclassOf) == 'function' and self.class:isSubclassOf(aClass)
        )
        )
    end,

    static = {
        allocate = function(self)
            assert(type(self) == 'table', "Make sure that you are using 'Class:allocate' instead of 'Class.allocate'")

            return setmetatable({ class = self }, self.__instanceDict)
        end,

        new = function(self, ...)
            local instance = self:allocate()
            
            assert(type(self) == 'table', "Make sure that you are using 'Class:new' instead of 'Class.new'")
            
            instance:initialize(...)
            
            return instance
        end,

        subclass = function(self, name)
            local subclass = _createClass(name, self)

            assert(type(self) == 'table', "Make sure that you are using 'Class:subclass' instead of 'Class.subclass'")
            assert(type(name) == "string", "You must provide a name(string) for your class")

            for methodName, f in pairs(self.__instanceDict) do
                if not (methodName == "__index" and type(f) == "table") then
                    _propagateInstanceMethod(subclass, methodName, f)
                end
            end

            subclass.initialize = function(instance, ...)
                return self.initialize(instance, ...)
            end

            self.subclasses[subclass] = true
            self:subclassed(subclass)

            return subclass
        end,

        subclassed = function(self, other)
        end,

        isSubclassOf = function(self, other)
            return type(other) == 'table' and type(self.super) == 'table' and (self.super == other or self.super:isSubclassOf(other))
        end,

        include = function(self, ...)
            assert(type(self) == 'table', "Make sure you that you are using 'Class:include' instead of 'Class.include'")
            
            for _, mixin in ipairs({...}) do 
                _includeMixin(self, mixin) 
            end
            
            return self
        end
    }
}
  

function middleclass.class(name, super)
    assert(type(name) == 'string', "A name (string) is needed for the new class")

    return super and super:subclass(name) or _includeMixin(_createClass(name), DefaultMixin)
end

setmetatable(middleclass, {
     __call = function(_, ...) 
        return middleclass.class(...) 
    end
})

class=middleclass

print("middleclass "..tostring(class))