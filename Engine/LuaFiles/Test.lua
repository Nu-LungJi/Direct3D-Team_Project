local Script = {}

function Script.OnCreate(ctx)
    print("OnCreate")
    print(ctx.ownerHandle)
end

function Script.OnDestroy(ctx)
    print("OnDestroy")
end

function Script.Update(ctx, dt)
    --print(dt)
end

function Script.PriorityUpdate(ctx, dt)
	--print("hihi")

    if true then
        if Input.KeyDown(Key.Space) then
            print("Space Down")
        end

        if Input.KeyPressing(Key.W) then
            --print("W Pressing")
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
end

return Script
