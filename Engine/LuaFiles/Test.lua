
function OnCreate()
    print("OnCreate")
    print(self)
    print(gameObject)
    print(transform:GetPosition())
end

function Update(dt)
    --print(dt)
end

function PriorityUpdate(dt)
	--print(dt)
	local x = 10;
    if false then
        if Input.KeyDown(Key.Space) then
            --print("Space Down")
        end

        if Input.KeyPressing(Key.W) then
            --print("W  Pressing")
        end

        if Input.KeyUp(Key.Escape) then
            --print("Escape Up")
        end

        if Input.MouseDown(Mouse.Left) then
            --print("Left Click")
        end

        local dx = Input.MouseMove(MouseMove.X)
        local dy = Input.MouseMove(MouseMove.Y)

        if dx ~= 0 or dy ~= 0 then
            --print("Mouse:", dx, dy)
        end
    end



    do
        if Input.KeyPressing(Key.Left) then
            transform:AddPosition(Vector3(-0.1, 0.0, 0.0))
        end

        if Input.KeyPressing(Key.Right) then
            transform:AddPosition(Vector3(0.1, 0.0, 0.0))
        end

        if Input.KeyPressing(Key.Up) then
            transform:AddPosition(Vector3(0.0, 0.0, 0.1))
        end

        if Input.KeyPressing(Key.Down) then
            transform:AddPosition(Vector3(0.0, 0.0, -0.1))
        end
    end
    
end
