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

function updateName()
    local player = g_game.getLocalPlayer()
    if player then
        local char = g_game.getCharacterName()
        print("Atualizando nome do jogador: " .. char)

        local playerNameLabel = healthBarWindow:recursiveGetChildById(
                                    'playerName')
        if playerNameLabel then playerNameLabel:setText(char) end
    end
end

function updateHealth()
    local player = g_game.getLocalPlayer()
    if not player or not healthBarWindow then return end

    local hp = player:getHealth()
    local maxHp = player:getMaxHealth()
    local healthPercent = (hp / maxHp) * 100

    local healthFill = healthBarWindow:recursiveGetChildById('healthFill')
    if healthFill then animateHealthBar(healthFill, healthPercent, 250) end

    local hpLabel = healthBarWindow:recursiveGetChildById('hpText')
    if hpLabel then hpLabel:setText(string.format("%d%%", healthPercent)) end
end

function updateLevel()
    local player = g_game.getLocalPlayer()
    if not player or not healthBarWindow then return end

    local levelLabel = healthBarWindow:recursiveGetChildById('playerLevel')
    if levelLabel then levelLabel:setText("Level: " .. player:getLevel()) end
end

function expForLevel(level)
    return math.floor(
               (50 * level ^ 3) / 3 - 100 * level ^ 2 + (850 * level) / 3 - 200)
end

function updateExperience()
    local player = g_game.getLocalPlayer()
    if not player or not healthBarWindow then return end

    local expBar = healthBarWindow:recursiveGetChildById('expBar')
    local expFill = healthBarWindow:recursiveGetChildById('expFill')
    local expText = healthBarWindow:recursiveGetChildById('expText')

    local rawExpPercent = player:getLevelPercent() or 0
    local expPercent = math.max(0, rawExpPercent)
    local exp = player:getExperience()
    local nextLevelExp = expForLevel(player:getLevel() + 1)
    local remainingExp = player:expToAdvance()
    local tooltipText = string.format("%d to next level", remainingExp)

    print("Atualizando experiência do jogador: " .. expPercent)

    if expFill then animateHealthBar(expFill, expPercent, 250) end

    if expText then
        expText:setText(string.format("%d%%", expPercent))
        expBar:setTooltip(tooltipText)
    end
end

function onPlayerLevelChange(player, level, percent)
    updateLevel()

    if not healthBarWindow then return end

    local expFill = healthBarWindow:recursiveGetChildById('expFill')
    local expText = healthBarWindow:recursiveGetChildById('expText')

    if not expFill or not expText then return end

    local expPercent = math.max(0, percent)
    local exp = player:getExperience()
    local nextLevelExp = expForLevel(level + 1)

    animateHealthBar(expFill, expPercent, 250)
    expText:setText(string.format("%d%%", expPercent))

end

function loadHealthBar()
    if healthBarWindow then
        healthBarWindow:destroy()
        healthBarWindow = nil
    end

    healthBarWindow = g_ui.loadUI('healthbar.otui',
                                  modules.game_interface.getRightPanel())
    if not healthBarWindow then
        perror("Falha ao carregar healthbar.otui")
        return
    end

    modules.game_interface.getRootPanel():addChild(healthBarWindow)

    updateName()
    updateHealth()
    updateLevel()
    updateExperience()

    healthBarWindow:show()
end

function init()
    connect(LocalPlayer, {
        onHealthChange = updateHealth,
        onLevelChange = updateLevel,
        onExperienceChange = updateExperience
    })

    connect(g_game, {
        onLogin = function() addEvent(function() loadHealthBar() end) end,
        onGameEnd = terminate
    })

    -- Caso esteja online já ao carregar
    if g_game.isOnline() then addEvent(function() loadHealthBar() end) end
end

function terminate()
    disconnect(LocalPlayer, {
        onHealthChange = updateHealth,
        onLevelChange = onPlayerLevelChange,
        onExperienceChange = updateExperience
    })

    if healthBarWindow then
        healthBarWindow:destroy()
        healthBarWindow = nil
    end
end
