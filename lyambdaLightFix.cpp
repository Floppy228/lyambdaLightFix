#include <string>
#include <windows.h>
using namespace std;

wstring program_name = L"lyambdaLightFix v0.2";
HHOOK hHook = NULL;
HWND h_class;
short timer_sec = -1; // Время таймера. -1 считается как дефолтное и не требует действий

// Горячая клавиша Ctrl+Alt+l
DWORD WINAPI HotKeyExit(LPVOID t) {
    while (true) {
        Sleep(100);
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000 &&  // Проверка Ctrl
            GetAsyncKeyState(VK_MENU) & 0x8000 &&     // Проверка Alt
            GetAsyncKeyState('L') & 0x8000) {         // Проверка L
            MessageBoxW(NULL, L"Программа завершена.", program_name.c_str(), MB_OK);
            exit(0);
        }
    }
}

// Функция для отправки клавиши в элемент управления
void SendKeyToHWND(HWND hwnd, WPARAM key) {
    if (!PostMessage(hwnd, WM_KEYDOWN, key, 0) || // Нажатие клавиши
        !PostMessage(hwnd, WM_KEYUP, key, 0))     // Отпускание клавиши
    {
   
        MessageBoxW(NULL, L"Ошибка выполнения. Возможно, Gaming Center был закрыт.", program_name.c_str(), MB_ICONERROR);
        exit(1);
    }
}

// Таймер, который прожимает кнопку по истечении своего времени
DWORD WINAPI Timer(LPVOID t) {
    while (true) {
        if (timer_sec > 0)
            timer_sec -= 1; // Уменьшаем таймер
        Sleep(1000); // Вместе с работой программы получается примерно секунда

        // Если время вышло, то жмем на кнопку,
        // инициализируя программу контроллера подсветки
        if (timer_sec == 0) {
            Sleep(800);
            // Чтобы избежать раннего выполнения, прожимаем несколько раз
            for (int i = 0; i < 5; ++i){
                SendKeyToHWND(h_class, VK_RETURN);
                Sleep(50);
            }
            timer_sec = -1; // Установка таймера в дефолтное значение
        }
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) { // Есть что считывать
        // Нажата либо символьная клавиша, либо системная
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
            // Клавиша не должна быть зажатой в данный момент
            if (!(GetAsyncKeyState(pKeyboard->vkCode) & 0x8000))
                timer_sec = 58; // таймер на 59 секунд
        }
    }
    // Отдаем хук дальше по цепочке
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Создаем поток горячей клавиши
    if (!CreateThread(NULL, 0, HotKeyExit, NULL, 0, NULL)) {
        MessageBoxW(NULL, L"Не удалось создать поток HotKeyExit.", program_name.c_str(), MB_ICONERROR);
        return 1;
    }
    SetThreadPriority(HotKeyExit, THREAD_PRIORITY_IDLE);

    // Создаем поток тайемера
    if (!CreateThread(NULL, 0, Timer, NULL, 0, NULL)) {
        MessageBoxW(NULL, L"Не удалось создать поток Timer.", program_name.c_str(), MB_ICONERROR);
        return 1;
    }
    SetThreadPriority(Timer, THREAD_PRIORITY_TIME_CRITICAL);

    wstring init_warning = L"Данная программа предназначена для обхода неработающего режима постоянной подсветки "\
        L"клавиатуры на ноутбуке LYAMBDA LLT161 16.1″. Перед выполнением убедитесь, что заводская программа "\
        L"Gaming Center запущена и находится в начальном состоянии. Не взаимодействуйте с ней во время выполнения "\
        L"данной программы. Для завершения работы нажмите сочетание клавиш Ctrl+Alt+l, появится окно об успешном"\
        L" выходе.\nВсе дальнейшие действия выполняются на ваши страх и риск.";
    MessageBoxW(NULL, init_warning.c_str(), program_name.c_str(), MB_OK);

    // Окно программы Gaming Center
    wstring window_name = L"WinUI Desktop";
    wstring error_message_find = L"Окно " + (wstring)window_name + L" не найдено. Gaming Center должен быть запущен.";
    HWND h_window = FindWindowW(NULL, window_name.c_str());
    if (!h_window) {
        MessageBoxW(NULL, error_message_find.c_str(), program_name.c_str(), MB_ICONERROR);
        return 1;
    }

    // Нужный класс, с которым будем работать впоследствии
    wstring class_name = L"Microsoft.UI.Content.ContentWindowSiteBridge";
    error_message_find = L"Класс " + (wstring)class_name + L" не найден";
    h_class = FindWindowExW(h_window, NULL, class_name.c_str(), NULL);
    if (!h_class) {
        MessageBoxW(NULL, error_message_find.c_str(), program_name.c_str(), MB_ICONERROR);
        return 1;
    }

    // Переход в меню управления подсветкой, выбор пункта Сохранить
    SendKeyToHWND(h_class, VK_DOWN);
    SendKeyToHWND(h_class, VK_DOWN);
    Sleep(2000); // Ждем пока прогрузится интерфейс
    for (int i = 0; i < 5; ++i) {
        SendKeyToHWND(h_class, VK_TAB);
    }

    // Получение хука клавиатуры
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), NULL);
    if (hHook == NULL) {
        MessageBoxW(NULL, L"Не удалось получить хук клавиатуры.", program_name.c_str(), MB_ICONERROR);
        exit(1);
    }

    // Бесконечный цикл проверки нажатия
    MSG Msg;
    while (true) {
        if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
            DispatchMessage(&Msg);
        }
    }
    UnhookWindowsHookEx(hHook); // Отдаем хук
    return 0;
}
