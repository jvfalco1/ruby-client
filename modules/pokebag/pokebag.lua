local window

function init()
  connect(g_game, { onGameEnd = hide })

  window = g_ui.displayUI('pokebag.otui')
  window:hide()
end

function terminate()
  disconnect(g_game, { onGameEnd = hide })
  if window then
    window:destroy()
  end
end

function show()
  if window then
    window:show()
    window:raise()
    window:focus()
  end
end

function hide()
  if window then
    window:hide()
  end
end
