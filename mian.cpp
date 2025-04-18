﻿#include "main.h"
#include <windows.h>

bool Flag_Dirty = false; //脏数据(未写入数据)标志位

/*Windows窗口大小*/
int Window_Width = 1280;
int Window_Height = 720;

static int Selected_Row = 0; //用户选中的行号

const std::string Config_Path = "Config/"; //用户文件存放路径
const std::string Config_File_Path = Config_Path + "Config.txt";//存放用户配置（账号）的文件

std::vector<Steam_Conf> Steam_Array;

//int main()
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    system("chcp 65001"); //终端设置UTF-8编码

    Read_Accounts_From_File(Config_File_Path);//读取文件内容到内存
    Check_Unban_Status();//检查封禁情况

    /*初始化GLFW*/
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    /*创建窗口*/
    GLFWwindow* window = glfwCreateWindow(Window_Width, Window_Height, "Steam_Mgr", nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);// 禁止用户调整窗口大小

    /*绑定OpenGL上下文*/
    glfwMakeContextCurrent(window);

    // **手动初始化 OpenGL 核心功能**
    gladLoadGL(glfwGetProcAddress);  // 添加这行代码

    /*初始化ImGui*/
    ImGui::CreateContext();//创建ImGui上下文
    ImGuiIO& io = ImGui::GetIO();// 获取 ImGui 的 IO 对象

    //设置微软雅黑字体, 并指定字体大小
    ImFont* font = io.Fonts->AddFontFromFileTTF
    (
        "Libs/Fonts/msyh.ttc",
        30,
        nullptr,
        //设置加载中文
        io.Fonts->GetGlyphRangesChineseFull()
    );

    //必须判断一下字体有没有加载成功
    IM_ASSERT(font != nullptr);

    // 应用字体到 ImGui
    io.FontDefault = font; // 设置默认字体

    /*初始化ImGui的渲染实现*/
    ImGui_ImplGlfw_InitForOpenGL(window, true);  // 使用 GLFW 和 OpenGL 初始化 ImGui
    ImGui_ImplOpenGL3_Init("#version 130");  // OpenGL 3.x 版本（你可以根据自己的 OpenGL 版本调整）

    /*设置垂直同步（可选）*/
    glfwSwapInterval(1);

    // 创建一个线程来定期执行检查任务
    std::jthread Check_Thread(Periodic_Check); //C++20 自动管理

    /*主循环（暂时只是保持窗口运行）*/
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents(); // 处理窗口事件

        /*开始新的一帧*/
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        /*绘制用户内容Begin*/
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);// 清空屏幕（背景色设为黑色）
        glClear(GL_COLOR_BUFFER_BIT);

        Draw_Table();//绘制表格
        Draw_Buttons();//绘制按钮
        Draw_Announcement();//绘制公告栏
        /*绘制用户内容End*/

        /*渲染 ImGui 内容*/
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());  // 渲染 ImGui 内容

        glfwSwapBuffers(window); // 交换前后缓冲

    }

    /*退出时释放资源*/
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    Check_Thread.request_stop();//通知定时检查线程结束

    Write_Accounts_To_File(Config_File_Path);//写入数据到文件

    return 0;
}

/*从文件读取数据到内存*/
void Read_Accounts_From_File(const std::string& File_Name)
{
    std::string Line;

    std::ifstream infile(File_Name);//打开文件（只读操作）

    int Num = 0;

    if (!infile)
    {
        std::cerr << "文件打开失败" << std::endl;
        return;
    }

    /*逐行读取文件内容*/
    while (std::getline(infile, Line))
    {
        size_t Pos1 = Line.find("----"); //返回第一个"----"的位置

        if (Pos1 != std::string::npos) //找到了第一个分隔符的位置
        {
            std::string User_Name = Line.substr(0, Pos1); //截取0到发现----的位置的字符串

            size_t Pos2 = Line.find("----", Pos1 + 4);  // 查找第二个 "----"

            if (Pos2 != std::string::npos)
            {
                std::string Password = Line.substr(Pos1 + 4, Pos2 - Pos1 - 4);  // 截取 Password

                size_t Pos3 = Line.find("----", Pos2 + 4);  // 查找第三个 "----"

                if (Pos3 != std::string::npos)
                {
                    // 获取 End_Time 和 Flag_Ban
                    std::string End_Time_Str = Line.substr(Pos2 + 4, Pos3 - Pos2 - 4);
                    std::string Flag_Ban_Str = Line.substr(Pos3 + 4);

                    // 转换 End_Time 和 Flag_Ban
                    std::time_t End_Time = std::stoll(End_Time_Str);  // 将 End_Time 转换为时间戳
                    int Flag_Ban = std::stoi(Flag_Ban_Str); //string to int

                    Steam_Conf Temp_Class(Num, User_Name, Password, End_Time, Flag_Ban);  // 创建一个新的 Steam_Conf 对象

                    Steam_Array.push_back(Temp_Class);  // 将其加入 Steam_Array 向量

                    Num++;  // 序号自增
                }
            }

        }

    }
    infile.close();  // 文件处理完毕后可以关闭文件

    std::cout << "数据成功读取：" << File_Name << std::endl;

}

/*写入数据到文件*/
void Write_Accounts_To_File(const std::string& File_Name)
{
    /*打开文件，如果文件不存在会创建一个新的 如果文件已经存在，它会清空文件并从文件开头写入新内容*/
    std::ofstream Out_file(File_Name, std::ios::out);

    if (!Out_file)
    {
        std::cerr << "文件打开失败" << std::endl;
        return;
    }

    for (int i = 0; i < Steam_Array.size(); i++)
    {
        Out_file << Steam_Array[i].User_Name << "----" << Steam_Array[i].Password << "----"
            << Steam_Array[i].End_Time << "----" << Steam_Array[i].Flag_Ban << std::endl;
    }

    Out_file.close();//关闭文件

    std::cout << "数据成功写入文件：" << File_Name << std::endl;
}

/*绘制表格*/
void Draw_Table(void)
{
    ImVec2 ImGui_Size = ImVec2(Window_Width - 50 - 350, Window_Height - 50);//右侧留出350放按钮

    /*设置表格窗口的位置和大小*/
    ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_Always);// 设定窗口位置 始终生效
    ImGui::SetNextWindowSize(ImGui_Size, ImGuiCond_Always);//设定窗口大小 始终生效

    /*禁止窗口的大小调整 去掉窗口的标题栏*/
    ImGui::Begin("Steam_Mgr", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        /*绘制边框 交替的背景色*/
        ImGui::BeginTable("账号列表", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImGui_Size);
        {
            /*设置列*/
            ImGui::TableSetupColumn("序号", ImGuiTableColumnFlags_WidthFixed, 50.0f);//设置列的宽度
            ImGui::TableSetupColumn("账号");//自动宽度
            ImGui::TableSetupColumn("密码");
            ImGui::TableSetupColumn("备注");

            /*绘制表头*/
            ImGui::TableHeadersRow();

            /*绘制数据行*/
            for (int i = 0; i < Steam_Array.size(); i++)
            {
                ImGui::TableNextRow();//开始一行

                ImGui::TableSetColumnIndex(0); // 第0列

                if (ImGui::Selectable(std::format("##row_{}", i).c_str(), Selected_Row == i, ImGuiSelectableFlags_SpanAllColumns))
                {
                    Selected_Row = i;

                    std::cout << "当前选中行：" << Selected_Row << std::endl;
                }
                ImGui::SameLine();

                ImGui::Text("%d", Steam_Array[i].Num); //显示序号

                ImGui::TableSetColumnIndex(1); // 第1列
                ImGui::Text(Steam_Array[i].User_Name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text(Steam_Array[i].Password.c_str());

                ImGui::TableSetColumnIndex(3);

                if (Steam_Array[i].Flag_Ban == 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));  // 绿色
                    ImGui::Text("可用");

                }
                else if (Steam_Array[i].Flag_Ban == 1)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));  // 红色
                    ImGui::Text("封禁");
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));  // 红色
                    ImGui::Text("永久");
                }

                ImGui::PopStyleColor();  // 恢复默认颜色
            }

        }
        ImGui::EndTable();
    }
    ImGui::End();

}

/*绘制按钮*/
void Draw_Buttons(void)
{
    //ImVec2 Temp_Pos;
    ImVec2 Button_Size;//按钮尺寸
    ImVec2 Button_Pos; //按钮位置

    ImVec2 ImGui_Size = ImVec2(350 - 25, (Window_Height - 50) / 20 * 12); //按钮界面尺寸（6/10）
    ImVec2 ImGui_Pos = ImVec2(Window_Width - 25 - ImGui_Size.x, 25);

    float Button_Width = 110.0f;
    float Button_Height = Button_Width / 1.618f;

    float Spacing_X = (ImGui_Size.x - (Button_Width * 2)) / 3; //计算按钮间距
    float Spacing_Y = (ImGui_Size.y - (Button_Height * 4)) / 5;

    /*设置按钮窗口的位置和大小*/
    ImGui::SetNextWindowPos(ImGui_Pos, ImGuiCond_Always);// 设定窗口位置 始终生效
    ImGui::SetNextWindowSize(ImGui_Size, ImGuiCond_Always);//设定窗口大小 始终生效

    /*禁止窗口的大小调整 去掉窗口的标题栏*/
    ImGui::Begin("Buttons", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        Button_Size = ImVec2(Button_Width, Button_Height);
        Button_Pos = ImVec2(Spacing_X, Spacing_Y);

        ImGui::SetCursorPos(Button_Pos);

        if (ImGui::Button("上号", Button_Size) == true) //上号
        {
            /*先关闭 Steam 和 PUBG 进程*/
            Terminate_Process("TslGame.exe");
            Terminate_Process("steam.exe");

            Steam_Array[Selected_Row].Login_Steam(); //登录Steam
        }

        ImGui::SameLine(Spacing_X + Button_Width + Spacing_X);//同行显示

        if (ImGui::Button("下号", Button_Size) == true) //下号
        {
            /*关闭 Steam 和 PUBG 进程*/
            Terminate_Process("TslGame.exe");
            Terminate_Process("steam.exe");
        }

        /*换行显示*/
        Button_Pos.x = Spacing_X;
        Button_Pos.y = Spacing_Y + (Button_Height + Spacing_Y) * 1;
        ImGui::SetCursorPos(Button_Pos);

        if (ImGui::Button("启动", Button_Size) == true)//启动游戏
        {
            //system("start steam://rungameid/578080");

            LaunchSteamGame(L"578080");// 启动 PUBG
        }

        ImGui::SameLine(Spacing_X + Button_Width + Spacing_X);//同行显示

        if (ImGui::Button("关闭", Button_Size) == true) //关闭游戏
        {
            /*关闭PUBG 进程*/
            Terminate_Process("TslGame.exe");
        }

        /*换行显示*/
        Button_Pos.x = Spacing_X;
        Button_Pos.y = Spacing_Y + (Button_Height + Spacing_Y) * 2;
        ImGui::SetCursorPos(Button_Pos);

        if (ImGui::Button("一天", Button_Size) == true)//检测一天
        {
            Steam_Array[Selected_Row].Set_Ban_Tim(24);
            Flag_Dirty = true;//脏数据 需要保存
        }

        ImGui::SameLine(Spacing_X + Button_Width + Spacing_X);//同行显示

        if (ImGui::Button("三天", Button_Size) == true)//检测三天
        {
            Steam_Array[Selected_Row].Set_Ban_Tim(72);
            Flag_Dirty = true;//脏数据 需要保存
        }

        Button_Pos.x = Spacing_X;
        Button_Pos.y = Spacing_Y + (Button_Height + Spacing_Y) * 3;

        ImGui::SetCursorPos(Button_Pos);

        if (ImGui::Button("永久", Button_Size) == true)//永久封禁
        {
            Steam_Array[Selected_Row].Flag_Ban = 2; //永久封禁
            Flag_Dirty = true;//脏数据 需要保存
        }

        ImGui::SameLine(Spacing_X + Button_Width + Spacing_X);//同行显示

        if (ImGui::Button("清除", Button_Size) == true)//清除
        {
            Steam_Array[Selected_Row].Flag_Ban = 0; //未封禁
            Flag_Dirty = true;//脏数据 需要保存
        }

    }
    ImGui::End();
}

void Draw_Announcement(void)
{
    ImVec2 ImGui_Size = ImVec2(350 - 25, ((Window_Height - 50) / 20 * 7)); //公告栏界面尺寸（1/4）
    ImVec2 ImGui_Pos = ImVec2(Window_Width - 25 - ImGui_Size.x, Window_Height - 25 - ImGui_Size.y);

    /*设置按钮窗口的位置和大小*/
    ImGui::SetNextWindowPos(ImGui_Pos, ImGuiCond_Always);// 设定窗口位置 始终生效
    ImGui::SetNextWindowSize(ImGui_Size, ImGuiCond_Always);//设定窗口大小 始终生效

    // 临时设置文本颜色
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(211, 211, 211, 211));

    /*禁止窗口的大小调整 去掉窗口的标题栏*/
    ImGui::Begin("Anno", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetCursorPos(ImVec2(25, 25));

        ImGui::Text("账号数据存放于\nConfig/Config.txt\n请按格式编辑\n\nQQ群: 1037699373\n软件版本: V1.0.0");
    }
    ImGui::End();

    ImGui::PopStyleColor();// 恢复默认的文本颜色
}

/*获取Steam路径*/
std::string Get_Steam_Path(void)
{
    HKEY hKey;
    const wchar_t* regPath = L"SOFTWARE\\Valve\\Steam";  // Steam注册表路径
    const wchar_t* regValue = L"SteamPath";              // Steam安装路径的键值

    // 打开注册表项
    if (RegOpenKeyEx(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        std::cerr << "Failed to open registry key!" << std::endl;
        return "";  // 如果打开失败，返回空字符串
    }

    wchar_t buffer[MAX_PATH];  // 用于存储Steam路径
    DWORD bufferSize = sizeof(buffer);

    // 查询Steam安装路径
    if (RegQueryValueEx(hKey, regValue, 0, NULL, (LPBYTE)buffer, &bufferSize) != ERROR_SUCCESS)
    {
        std::cerr << "Failed to query SteamPath!" << std::endl;
        RegCloseKey(hKey);
        return "";  // 如果查询失败，返回空字符串
    }

    RegCloseKey(hKey);  // 关闭注册表项

    // 将宽字符路径转换为标准字符串并返回
    std::wstring ws(buffer);
    std::string steamPath(ws.begin(), ws.end());

    // 返回获取到的Steam路径
    return steamPath;
}

/*登录steam*/
void Steam_Conf::Login_Steam(void)
{
    std::string Steam_Path = Get_Steam_Path();//获取Steam路径

    //std::cout << "Steam的路径：" << Steam_Path << std::endl;

    // 构建启动命令
    std::string command = Steam_Path + "\\steam.exe -login " + User_Name + " " + Password;

    // 将命令转换为宽字符字符串
    std::wstring wcommand(command.begin(), command.end());

    // 打印尝试登录的信息
    std::cout << "尝试登录 Steam..." << std::endl;

    // 使用线程来启动 Steam
    std::thread Steam_Thread(Launch_Process, wcommand);
    Steam_Thread.detach();  //让线程独立运行

    // 主程序继续执行，界面可以操作
    std::cout << "Steam 启动中..." << std::endl;

    // 如果需要等待线程完成，可以使用 join()
    //Steam_Thread.join();  // 等待线程完成，可以去掉这个，程序会直接执行完退出

}

/*设置封号时间*/
void Steam_Conf::Set_Ban_Tim(long long Hour)
{
    Flag_Ban = 1; //账号临时检测

    std::time_t Now = std::time(nullptr);//获取当前时间（从1970年1月1日起的秒数）
    End_Time = Now + (Hour * 3600);//1小时 == 3600秒
}

/*检查封禁情况*/
void Check_Unban_Status(void)
{
    std::time_t Now = std::time(nullptr);// 获取当前时间

    for (int i = 0; i < Steam_Array.size(); i++)//遍历所有账号
    {
        if (Steam_Array[i].Flag_Ban == 1) //如果当前账号被检测
        {
            if (Now > Steam_Array[i].End_Time) //封禁时间已过
            {
                Steam_Array[i].Flag_Ban = 0;//账号正常

                Flag_Dirty = true;//脏数据 需要保存
            }
        }
    }

}

/*定期检查任务*/
void Periodic_Check(std::stop_token token)
{
    while (!token.stop_requested())// ✅ 当 `stop_requested()` 返回 true，退出循环
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));  // 当前线程“睡眠”指定的时间

        Check_Unban_Status();//检查封禁情况

        if (Flag_Dirty == true) //数据已修改
        {
            Write_Accounts_To_File(Config_File_Path);//写入文件

            Flag_Dirty = false;
        }
    }

    std::cout << "定时检查线程退出" << std::endl;
}

/*创建线程*/
void Launch_Process(const std::wstring& command)
{
    // 创建一个进程信息结构体
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 将 std::wstring 转换为可修改的 wchar_t* 类型
    wchar_t* command_ptr = const_cast<wchar_t*>(&command[0]);

    // 使用 CreateProcess 启动 Steam
    if (!CreateProcessW(
        NULL,               // 可执行文件名
        command_ptr,        // 宽字符命令行参数
        NULL,               // 进程句柄不可继承
        NULL,               // 线程句柄不可继承
        FALSE,              // 不继承文件句柄
        0,                  // 默认创建新进程
        NULL,               // 使用当前环境
        NULL,               // 使用当前目录
        &si,                // 启动信息
        &pi)                // 进程信息
        )
    {
        std::cerr << "创建进程失败，错误代码：" << GetLastError() << std::endl;
    }
    else
    {
        // 进程创建成功，可以继续执行后续代码
        std::cout << "进程启动命令已发送！" << std::endl;

        // 等待进程结束
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// 结束指定的进程
void Terminate_Process(const std::string& processName)
{
    std::cout << "正在关闭 " << processName << " ..." << std::endl;
    std::string command = "taskkill /f /im " + processName;  // 构造命令
    system(command.c_str());  // 执行命令
}

/*启动Steam游戏*/
void LaunchSteamGame(const std::wstring& game_id)
{
    // 启动 游戏的命令
    std::wstring command = L"steam://rungameid/" + game_id;

    // 使用 ShellExecute 启动 Steam的游戏
    if (ShellExecuteW(NULL, L"open", command.c_str(), NULL, NULL, SW_SHOWNORMAL) > (HINSTANCE)32)
    {
        std::cout << "Steam 启动并启动 PUBG!" << std::endl;
    }
    else
    {
        std::cerr << "启动 Steam 失败，错误代码：" << GetLastError() << std::endl;
    }
}