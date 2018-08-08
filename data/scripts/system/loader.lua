function load_entity(data)
    local ent = entities:create_entity()
    for k,v in pairs(data) do
        if component[k] ~= nil then
            local com = component[k].new()
            if v ~= nil then
                for k,v in pairs(v) do
                    com[k] = v
                end
            end
            entities:create_component(ent, com)
        else
            print("Unknown component: "..k)
        end
    end
    return ent
end

function load_world(data)
    print("Loading stage")
    for k,v in ipairs(data) do
        load_entity(v)
    end
end
