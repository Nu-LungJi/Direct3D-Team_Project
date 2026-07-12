
function OnCreate()
    print("OnCreate")
end

function Update(dt)
end

function PriorityUpdate(dt)
    do
        if  Input.KeyDown(Key.L) then
            --Level.ChangeLevel("TO_LOGO")
        end

        if Input.KeyDown(Key.C) then
            local mainCamera = Camera.GetActiveCamera()
            local rayOrigin, rayDir = mainCamera:GetRay()
            print(mainCamera)
            print("Ray 시작점 X: " .. rayOrigin.x)
            print("Ray 방향 Z: " .. rayDir.z)

            local testPos = rayOrigin + (rayDir * 10.0)
            print("testPos : " .. tostring(testPos) )
        end

        if Input.KeyDown(Key.Space) then
            --msgBox("HKKAKAK")
            --gameObject:DestroyCascade(true)

            -- 01_COLLIDERS
            local monsterList = Object.GetLayer("01_COLLIDERS")
            for i, monster in ipairs(monsterList) do
                print(monster)
                
                local handle = monster:GetHandle()
                local testObj = Object.GetByHandle(handle)

                if testObj ~= nil then
                    -- 루아에서 문자열 연결은 +가 아니라 .. 입니다.
                    print("GetByHandle 테스트 성공: " .. tostring(testObj))
                    --testObj:DestroyCascade(true)
                else
                    print("GetByHandle 테스트 실패: 객체를 찾을 수 없습니다.")
                end

                -- 이제 monster는 C++에서 등록된 타입(예: CMonster)으로 인식됩니다.
                local transform = monster:GetComponent("Com_Transform")
                print(transform)
                if transform ~= nil then
                    print(i .. "번째 몬스터의 위치: " .. transform:GetPosition().x)
                end
            end
        end

        if Input.KeyPressing(Key.Left) then
            transform:AddPosition(Vector3.new(-0.1, 0, 0))
            --print("yyy")
        end

        if Input.KeyPressing(Key.Right) then
            transform:AddPosition(Vector3.new(0.1, 0.0, 0.0))
        end

        if Input.KeyPressing(Key.Up) then
            transform:AddPosition(Vector3.new(0.0, 0.0, 0.1))
        end

        if Input.KeyPressing(Key.Down) then
            transform:AddPosition(Vector3.new(0.0, 0.0, -0.1))
        end
    end
    
end


function Update(dt)
    
    DbgLine.AddCapsule(1,2,transform:GetCombinedWorldMatrix())
end


function LateUpdate(dt)
    if Input.MouseDown(Mouse.Left) then
        --transform:AddPosition(Vector3.new(0.1, 0.0, 0.0))
        local currcollider = gameObject:GetComponent("ComCollider1")
        print(currcollider)
        local monsterList = Object.GetLayer("01_COLLIDERS")
        for i, monster in ipairs(monsterList) do
            
            local coll = monster:GetComponent("ComCollider1")
            if(currcollider:Get():Intersect(coll:Get()      )) then
                local targetObj = coll:GetGameObject();
                targetObj:DestroyCascade(true);
            end
        end 
    end

    if Input.MouseDown(Mouse.Right) then
        local pos = transform:GetPosition();    
        Gen.Spawn(pos.x + Rand.Randf(-1,1), pos.y + Rand.Randf(-1,1), pos.z +        Rand.Randf(-1,1))     
    end
end
