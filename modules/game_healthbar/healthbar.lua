local healthBarWindow = nil

function animateHealthBar(widget, targetPercent, duration)
    if not widget then return end

    local startWidth = widget:getWidth()
    local parentWidth = widget:getParent():getWidth()
    local targetWidth = math.floor(parentWidth * targetPercent / 100)
    local startTime = g_clock.millis()
    local endTime = startTime + duration

    local function step()
        local now = g_clock.millis()
        local t = math.min((now - startTime) / duration, 1.0)
        local currentWidth = math.floor(
                                 startWidth + (targetWidth - startWidth) * t)
        widget:setWidth(currentWidth)

        if t < 1.0 then scheduleEvent(step, 10) end
    end

    step()
end

function updateHealth()
    local player = g_game.getLocalPlayer()
    if not player or not healthBarWindow then return end

    local hp = player:getHealth()
    local maxHp = player:getMaxHealth()
    local healthPercent = (hp / maxHp) * 100

    local healthFill = healthBarWindow:recursiveGetChildById('healthFill')
    if healthFill then
        local parentWidth = healthFill:getParent():getWidth()
        local targetWidth = math.floor(parentWidth * (healthPercent / 100))
        animateHealthBar(healthFill, healthPercent, 250)
    end

    local hpLabel = healthBarWindow:recursiveGetChildById('hpLabel')
    if hpLabel then hpLabel:setText(string.format("%d / %d HP", hp, maxHp)) end
end

function updateLevel()
    local player = g_game.getLocalPlayer()
    if not player or not healthBarWindow then return end

    local levelLabel = healthBarWindow:recursiveGetChildById('playerLevel')
    if levelLabel then levelLabel:setText("Level: " .. player:getLevel()) end
end

function updateExperience()
    local player = g_game.getLocalPlayer()
    if not player or not healthBarWindow then return end

    local expFill = healthBarWindow:recursiveGetChildById('expFill')
    local expText = healthBarWindow:recursiveGetChildById('expText')

    local expPercent = player:getLevelPercent() or 0

    if expFill then
        local parentWidth = expFill:getParent():getWidth()
        local targetWidth = math.floor(parentWidth * (expPercent / 100))
        animateHealthBar(expFill, expPercent, 250)
    end

    if expText then
        local exp = player:getExperience()
        local nextLevelExp = expForLevel(player:getLevel() + 1)
        expText:setText(string.format("%d / %d EXP", exp, nextLevelExp))
    end
end

function init()
    print("Iniciando modulo healthbar...")
    connect(LocalPlayer, {
        onHealthChange = updateHealth,
        onLevelChange = updateLevel,
        onExperienceChange = updateExperience
    })

    addEvent(function()
        print("Testando acesso ao arquivo OTUI")
        print("Arquivo relativo existe?",
              g_resources.fileExists("healthbar.otui"))

        healthBarWindow = g_ui.loadUI('healthbar.otui',
                                      modules.game_interface.getRightPanel())
        if not healthBarWindow then
            perror("Falha ao carregar healthbar.otui")
            return
        end

        modules.game_interface.getRootPanel():addChild(healthBarWindow)
        updateHealth()
        updateLevel()
        updateExperience()
        healthBarWindow:show()
    end)
end

function terminate()
    disconnect(LocalPlayer,
               {onHealthChange = updateHealth, onLevelChange = updateLevel})

    if healthBarWindow then
        healthBarWindow:destroy()
        healthBarWindow = nil
    end
end
