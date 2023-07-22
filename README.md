# Периметр 2: Новая земля

![Perimeter2](https://cdn.cloudflare.steamstatic.com/steam/apps/12420/header.jpg?t=1574188988)

[![Discord Maelstrom](https://img.shields.io/badge/Discord-Maelstrom-5765ec?logo=discord&logoColor=white)](https://discord.gg/rUcXXCY)
[![Telegram Maelstrom Chat](https://img.shields.io/badge/Telegram-Maelstrom_Chat-35ade1?logo=telegram)](https://t.me/maelstrom2007chat)
[![Discord Vista Engine](https://img.shields.io/badge/Discord-Vista_Engine-5765ec?logo=discord&logoColor=white)](https://discord.gg/Tfx49kBZPF)
[![Telegram Vista Engine Chat](https://img.shields.io/badge/Telegram-Vista_Engine_Chat-35ade1?logo=telegram)](https://t.me/vistaengine)
[![Telegram Perimeter Chat](https://img.shields.io/badge/Telegram-Perimeter_Chat-35ade1?logo=telegram)](https://t.me/PerimeterGame)

Для запуска игры требуются ресурсы (карты, звуки, текстуры и т.д.), которые имеются в приобретенной цифровой/физической версии игры.

**Ветки:**\
[`Vista Engine`](https://github.com/KD-lab-Open-Source/VistaEngine) | [`Perimeter 2`](https://github.com/KD-lab-Open-Source/VistaEngine/tree/Perimeter2) [`Maelstrom (v1.1)`](https://github.com/KD-lab-Open-Source/VistaEngine/tree/Maelstrom_1_1) | [`Maelstrom (v1.0)`](https://github.com/KD-lab-Open-Source/VistaEngine/tree/Maelstrom)

## Лицензии
(с) ООО "КД ВИЖЕН" (Калининград)

Весь код, за исключением сторонних библиотек, публикуется под лицензией GPLv3. Код сторонних библиотек (где указана иная лицензия) публикуется под лицензией этих библиотек.

## Работоспособность кода
* Не работает мультиплеер по локальной сети (был вырезан DemonWare из OpenSource).
* Не работает онлайн-мультиплеер (был вырезан DemonWare из OpenSource).
* Не работают видеоролики (был вырезан Bink из OpenSource).

## Состав репозитория
* / - Основной код игры.
* /AI - Содержит код, отвечающий за искусственный интеллект.
* /AttribEditor - Содержит некоторый код пользовательского интерфейса Vista Engine.
* /Configurator - Графическая утилита, поставляемая вместе с игрой, которая позволяет игрокам изменять язык и другие настройки.
* /EasyMap - Утилиты для тестирования производительности движка.
* /EFFECTTOOL - Инструмент для редактирования эффектов.
* /Environment - Содержит код окружающей среды.
* /Game - Содержит точку входа для игры в файле Runtime.cpp и другой связанный с игрой код.
* /HT - Сокращение от "HyperThreading" (Гиперпоточность). Содержит некоторые элементы, связанные с многопоточностью.
* /IGameExporter2 - Плагин для экспорта моделей в формате 3dx.
* /Network - Сетевой код игры и движка.
* /Render - Содержит код рендеринга 3dx и графики.
* /Sound - Содержит код для работы с звуковыми эффектами и управления музыкой.
* /SurMap5 - Vista Engine.
* /SurMap5/ProfUIS - Директория сторонней библиотеки [Prof-UIS](#required).
* /Terra - Содержит код, отвечающий за терраформирование ландшафта и загрузку миров.
* /TriggerEditor - Графический интерфейс для редактирования цепочек триггеров.
* /UIEditor - Инструмент для редактирования пользовательского интерфейса.
* /Userinterface - Содержит код, обрабатывающий пользовательский интерфейс игры.
* /Util - Утилиты для игры и других модулей.
* /Water - Содержит код живого мира и природных явлений.
* /WinVG - Инструмент для просмотра моделей в формате 3dx.
* /XLibs.Net - Библиотеки используемые игрой.
* /XLibs.Net/MSDXSDK_02_06 - DirectX SDK используемый игрой.
* /ZipPacker - Инструмент для упаковки файлов в формате pak.

## <a name="required"></a>Что потребуется
Проверено что игра собирается в окружении Windows 10 (22H2) / Windows 11 (22H2) + Visual Studio 2003.
* Для сборки `VistaEngine` обязательно потребуется `Prof-UIS` 710-й версии.
* Для сборки `IGameExporter2` потребуется `maxsdk` от 3dsMax 7/8.

**Prof-UIS:**\
[`Prof-UIS Freeware v.2.93`](https://web.archive.org/web/20120521192708/http://www.prof-uis.com/download/profuis293_freeware.zip "Скачать Prof-UIS Freeware v.2.93")

## Сборка
Откройте любой .sln файл, затем нажмите Build > Rebuild Solution.\
После первой пересборки, можете использовать Build > Build Solution.
* /`Perimeter2.sln` - Игра.
* /Configurator/`Configurator.sln` - Утилита для смены языка и настроек игры.
* /EasyMap/`EasyMap.sln` - Утилита для тестирования производительности движка.
* /EFFECTTOOL/`EffectTool.sln` - Редактор эффектов.
* /IGameExporter2/`IGameExporter.sln` - Плагин для экспорта 3dx.
* /SurMap5/`SurMap5.sln` - Vista Engine.
* /UIEditor/`UIEditor.sln` - Редактор интерфейса.
* /WinVG/`WinVG.sln` - Просмоторщик 3dx.

## Запуск игры
Скопируйте собранный .exe файл (Game.exe, VistaEngine.exe или любой другой), а также AttribEditor.dll и TriggerEditor.dll в папку с игрой.
