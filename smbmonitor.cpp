#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <thread>

#include <stdio.h>
#include <unistd.h>
#include <bits/stdc++.h> 

#include "headers/chroma.h"
#include "headers/libled.h"
#include "headers/PCA9685.h"

using namespace std;

typedef enum
{
    OFF,
    ON,
    VIDS
}sstatus;


int client_num = 0;
bool streaming_vid;
set<uint32_t> client_list;
vector<string> locked_files;
sstatus smb_status = OFF;
sstatus smb_status_old = OFF;

uint32_t ip_to_int(string address)
{
    uint32_t ret = 0;
    ushort byte;
    char dot;
    stringstream ss(address);
    for (int i = 3; i >= 0; i--)
    {
        ss >> byte;
        ss >> dot;
        ret += byte << i * 8;
    }
    return ret;
}

string int_to_ip(uint32_t address)
{
    string ret = to_string((address >> 24) & 0xFF);
    for (int i = 2; i >= 0; i--)
        ret += ("." + to_string((address >> i * 8) & 0xFF));
    return ret;
}

int get_smbstatus_output(string &result)
{
    result = "";
    char buffer[128];
    FILE* pipe = popen("/usr/bin/smbstatus", "r");
    if (!pipe)
        return -1;
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
          result += buffer;
    }
    pclose(pipe);
  
    if (result == "smbstatus only works as root!\n")
    {
        cout << "Root is required to run this program" << endl;
        return -1;
    }
    return 0;
}

set<uint32_t> list_connected_ip(stringstream &ss)
{
    set<uint32_t> ret;
    string line, tmp, address;
    stringstream ss_line;
    for (int i = 0; i < 4; i++)
        getline(ss, line);
    while(1)
    {
        getline(ss, line);
        if (line == "")
            break;
        ss_line = stringstream(line);
        for (int i = 0; i < 3; i++)
            ss_line >> tmp;
        ss_line >> address;
        ret.insert(ip_to_int(address));
    }
    return ret;
}

string file_format(string &file)
{
    int i = file.size() - 1;
    while(i >= 0 && file[i] != '.')
        i--;
    if (i >= 0 && i < file.size() - 1)
        return string(file.begin() + i + 1, file.end());
    return "";
}

inline bool is_video(string format)
{
    if (format == "mp4" ||
        format == "mkv" ||
        format == "rmvb" ||
        format == "avi" ||
        format == "wmv")
        return true;
    return false;
}

vector<string> list_locked_files(stringstream &ss)
{
    string line;
    getline(ss, line);
    if (line == "No locked files")
        return {};
    
    string tmp, file_path, file_name;
    stringstream ss_line;
    vector<string> ret;
    getline(ss, line);
    getline(ss, line);
    while(1)
    {
        getline(ss, line);
        if (line == "")
            break;
        line = string(line.begin(), line.end() - 25);
        ss_line = stringstream(line);
        for (int i = 0; i < 6; i++)
            ss_line >> tmp;
        ss_line >> file_path;
        ss_line >> file_name;
        while(ss_line >> tmp)
            file_name = file_name + " " + tmp;
        ret.push_back(file_path + "/" + file_name);
        if (is_video(file_format(file_name)))
            streaming_vid = true;
    }
    return ret;
}

void update_status(string &smb_output)
{
    client_num = 0;
    client_list = {};
    streaming_vid = false;
    locked_files = {};
    smb_status_old = smb_status;

    string tmp;
    stringstream smb_output_ss(smb_output), line_ss;
    client_list = list_connected_ip(smb_output_ss);
    client_num = client_list.size();
    do getline(smb_output_ss, tmp); while (tmp != "");
    locked_files = list_locked_files(smb_output_ss);

    if (streaming_vid)
        smb_status = VIDS;
    else
    {
        if (client_num > 0)
            smb_status = ON;
        else
            smb_status = OFF;
    }
}

bool stop_streaming()
{
    return smb_status != VIDS;
}

int main(){
    PCA9685 hat(0x40, 1);
    hat.init();
    hat.setFreq(1525);

    chromled led0(&hat, 0, 1, 2, COMMON_ANODE);
    chromled led1(&hat, 3, 4, 5, COMMON_ANODE);

    vector<chromled*> leds{&led0, &led1};
    chroma_layer layer(leds);
    
    srand(time(NULL));
    uint rand_i;
    vector<RGB_12bit> rainbow_colors({RED, ORANGE, YELLOW, GREEN, BLUE, VIOLET});
    vector<RGB_12bit> colors(leds.size(), GREEN);
    string smb_output;

    while(1)
    {
        if (get_smbstatus_output(smb_output) != 0)
            return -1;
        update_status(smb_output);

        if (smb_status_old == OFF && smb_status == ON)
        {   
            cout << "Server is ON" << endl;
            layer.load(blink_on, 40, 100);
            // rand_i = rand() % rainbow_colors.size();
            // for (RGB_12bit &c : colors)
            //     c = rainbow_colors[rand_i];
            layer.load(linear_gradient, colors, 1000);
        }
        else if (smb_status_old != OFF && smb_status == OFF)
        {
            cout << "Into sleep mode." << endl;
            layer.load(blink_off, 100, 80);
        }
        else if (smb_status_old != VIDS && smb_status == VIDS)
        {
            cout << "Sreaming videos..." << endl;
            layer.load(rainbow, stop_streaming, 256);
        }
        else if (smb_status_old != ON && smb_status == ON)
        {
            cout << "Server is idle" << endl;
            // rand_i = rand() % rainbow_colors.size();
            // for (RGB_12bit &c : colors)
            //     c = rainbow_colors[rand_i];
            layer.load(linear_gradient, colors, 512);
        }

        sleep(1);
        // cout << "\033c";
    }
    return 0;
}