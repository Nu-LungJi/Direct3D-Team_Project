print("ASDF!");
print(__ScriptPath);

function OnCreate()
    print("OnCreate")
    print(self)
    print(gameObject)
    print(transform:GetPosition())
end

function Update(dt)
    print(dt)
end
