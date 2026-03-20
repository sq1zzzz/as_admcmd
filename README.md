# [AS] Admin Commands

**RU** | [EN](#en)

---

## RU

Дополнительный модуль для [AdminSystem](https://github.com/Pisex/cs2-admin_system) от Pisex.  
Добавляет новую категорию **"Управление командами"** в меню `!admin`.

### Возможности

| Функция | Описание |
|---|---|
| Admin ESP | Видишь всех игроков через стены. Только ты — никто другой не видит |
| Возродить игрока | Возрождает любого игрока |
| Телепортировать игрока | Телепортирует одного игрока к другому |
| Выдать деньги | Выдаёт деньги ($2 000 – $16 000) |
| Отнять деньги | Отнимает деньги у игрока |
| Бесконечные патроны | Бесконечный боезапас для тебя |
| Выдать оружие | Выдаёт любое оружие из арсенала CS2 |
| Сбросить счёт | Обнуляет убийства, смерти, ассисты и очки |

### Почему два плагина?

Модуль состоит из двух частей:

`as_admcmd` — Metamod плагин.** Основная часть: меню, телепорт, деньги, оружие, сброс счёта, интеграция с AdminSystem.

`AdminESP` — CounterStrikeSharp плагин.** Отдельный плагин только для ESP. Реализовать настоящий ВХ (glow через стены) так, чтобы видел только один конкретный админ, в Metamod невозможно. CounterStrikeSharp даёт доступ к `CheckTransmit` — механизму который контролирует что именно отправляется каждому клиенту. Именно через него подсветка показывается только тому у кого включён ESP, а остальные игроки её не видят.

### Требования

- [Metamod:Source](https://www.sourcemm.net/downloads.php?branch=dev)
- [AdminSystem](https://github.com/Pisex/cs2-admin_system/releases/latest)
- [Utils](https://github.com/Pisex/cs2-menus/releases/tag/1.8.6.2)
- [CounterStrikeSharp](https://github.com/roflmuffin/CounterStrikeSharp/releases)

### Установка

**1. Скопируй файлы на сервер:**

```
game/csgo/addons/admin_modules/as_admcmd.so
game/csgo/addons/metamod/as_admcmd.vdf
game/csgo/addons/counterstrikesharp/plugins/AdminESP/AdminESP.dll
```

**2. Добавь переводы:**

Открой `csgo/addons/translations/admin_system.phrases.txt` и вставь содержимое файла `admin_system.phrases.txt` из архива перед последней закрывающей `}`.

**3. Добавь в sorting:**

Открой `csgo/addons/configs/admin_system/sorting.ini` и вставь содержимое файла `sorting.txt` из архива перед последней закрывающей `}`:

```
	"admin_commands"
	{
		"item"		"admin_commands"
	}
```

**4. Права доступа:**

Флаг `@admin/admcmd` нужным администраторам.

### Уведомления в чате

- **ESP, бесконечные патроны** — сообщение видит только сам админ
- **Когда ESP включается** — все с флагом `@admin/root` видят уведомление кто его включил
- **Возрождение, телепорт, деньги, оружие, счёт** — сообщение в общий чат

---

<a name="en"></a>
## EN

An additional module for [AdminSystem](https://github.com/Pisex/cs2-admin_system) by Pisex.  
Adds a new category **"Admin Commands"** to the `!admin` menu.

### Features

| Feature | Description |
|---|---|
| Admin ESP | See all players through walls. Only you — no one else can see it |
| Respawn Player | Respawn any player |
| Teleport Player | Teleport one player to another |
| Give Money | Give money to a player ($2,000 – $16,000) |
| Take Money | Take money from a player |
| Infinite Ammo | Unlimited ammo for yourself |
| Give Weapon | Give any weapon from the CS2 arsenal |
| Reset Score | Reset a player's kills, deaths, assists and score |

### Why two plugins?

**`as_admcmd` — Metamod plugin.** The main part: menu, teleport, money, weapons, score reset, AdminSystem integration.

**`AdminESP` — CounterStrikeSharp plugin.** A separate plugin for ESP only. It is not possible to implement a proper wallhack (player glow visible through walls) in Metamod in a way that only one specific admin sees it. CounterStrikeSharp provides access to `CheckTransmit` — a mechanism that controls exactly what is sent to each client. This is what makes the glow visible only to the admin with ESP enabled, while everyone else sees nothing.

### Requirements

- [Metamod:Source](https://www.sourcemm.net/downloads.php?branch=dev)
- [AdminSystem](https://github.com/Pisex/cs2-admin_system/releases/latest)
- [Utils](https://github.com/Pisex/cs2-menus/releases/tag/1.8.6.2)
- [CounterStrikeSharp](https://github.com/roflmuffin/CounterStrikeSharp/releases)

### Installation

**1. Copy files to server:**

```
game/csgo/addons/admin_modules/as_admcmd.so
game/csgo/addons/metamod/as_admcmd.vdf
game/csgo/addons/counterstrikesharp/plugins/AdminESP/AdminESP.dll
```

**2. Add translations:**

Open `csgo/addons/translations/admin_system.phrases.txt` and insert the contents of `admin_system.phrases.txt` from the archive before the last closing `}`.

**3. Add to sorting:**

Open `csgo/addons/configs/admin_system/sorting.ini` and insert the contents of `sorting.txt` from the archive before the last closing `}`:

```
	"admin_commands"
	{
		"item"		"admin_commands"
	}
```

**4. Access rights:**

Flag `@admin/admcmd` for required administrators.

### Chat notifications

- **ESP, infinite ammo** — message visible only to the admin who toggled it
- **When ESP is enabled** — all admins with `@admin/root` see a notification in chat
- **Respawn, teleport, money, weapon, score** — message goes to general chat

#Author:

sq1z
