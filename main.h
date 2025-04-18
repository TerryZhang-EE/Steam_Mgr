﻿#include <glad/gl.h>// Glad 头文件（OpenGL加载器）
#include <GLFW/glfw3.h>// GLFW 头文件

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <iostream> // C++ 标准库头文件
#include <fstream>     // 用于 std::ifstream
#include <string>      // 用于 std::string
#include <vector>
#include <format>
#include <thread>
#include <corecrt.h>

struct Steam_Conf
{
public:
    unsigned int Num;            // 序号
    std::string User_Name;       // 用户名
    std::string Password;        // 密码
    std::time_t End_Time = 0;//结束时间
    int Flag_Ban; //0:正常 1:检测 2:封禁

    void Login_Steam(void);
    void Set_Ban_Tim(long long Hour);

};

void Read_Accounts_From_File(const std::string& File_Name);//从文件读取数据到内存
void Write_Accounts_To_File(const std::string& File_Name);//从内存写入数据到文件
void Draw_Table(void);
void Draw_Buttons(void);
void Draw_Announcement(void);
std::string Get_Steam_Path(void); //获取Steam路径
void Check_Unban_Status(void);//遍历检查账号封禁情况
void Periodic_Check(std::stop_token token);//定期检查任务
void Launch_Process(const std::wstring& command);//创建指定进程
void Terminate_Process(const std::string& processName);// 结束指定的进程
void LaunchSteamGame(const std::wstring& game_id);//启动Steam的游戏