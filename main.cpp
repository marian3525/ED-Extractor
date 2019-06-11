#include <iostream>
#include <string>
#include <vector>
#include "pstream.h"
#include <fstream>
#include <map>
#include <cstring>
#include <algorithm>

using namespace std;

map<string, int> friends;


string processChatLine(const char* line) {
    /*
     * For messages sent: [time] [Marian] [MessageText]
     *              received: [time] [sender] [MessageText]
     */
    string result;
    string time;

    const char* ptr = strchr(line, '{');

    while(*ptr != '}' and *ptr != 0) {
        time += *ptr;
        ptr++;
    }
    time += *ptr;

    result += time + " ";

    if(strstr(line, " (received) ") != nullptr) {
        // received a message, sender name should be in the line
        // name starting at 63, ending at ':'
        const char* pos = line + 63;
        string name;

        while(*pos != ':' and *pos != 0) {
            name.push_back(*pos);
            pos++;
        }

        // got the name
        std::remove(name.begin(), name.end(), ' ');
        result += "[received] [" + name +"] ";
        friends[name]++;


        pos += 2;
        string message;
        while(*pos != 0 and *pos != '\r' and *pos != 0) {
            message.push_back(*pos);
            pos++;
        }

        // remove the trailing '
        message.pop_back();
        result += message;
    }
    else {
        // sent message, receiver is given as a list of numbers
        const char* pos = line + 56;
        string serial;

        while(*pos != ']' and *pos != 0) {
            serial.push_back(*pos);
            pos++;
        }

        // got the serial
        result += "[sent] [" + serial +"] ";

        pos += 4;

        string message;
        while(*pos != 0 and *pos != '\r') {
            message.push_back(*pos);
            pos++;
        }

        result += message;
    }

    // identity extracted, now for the message

    return result;
}

string getFormattedDate(string numbers) {
    // input as netlog.yymmddxyasdasda
    // extract year, month, day
    char* start = strchr(numbers.data(), '.');

    string output;
    output += *(start + 5);
    output += *(start + 6);
    output += ".";
    output += *(start + 3);
    output += *(start + 4);
    output += ".";
    output += *(start + 1);
    output += *(start + 2);

    return output;
}

vector<string> getFiles(string input) {
    /*
     * Feed the input into a ls and grab the output and return the list of files
     */
    vector<string> files;

    // using elite_logs as testing folder, it has less data
    string command = "ls " + input + "/*";

    // run a process and create a streambuf that reads its stdout and stderr
    redi::ipstream proc(command, redi::pstreams::pstdout);
    std::string line;

    // read child's stdout
    while (std::getline(proc.out(), line)) {
        std::cout << "File found: " << line << '\n';
        files.push_back(line);
    }
    return files;
}

/*
 * When sending a message pattern:
 * {12:54:48GMT 3135.417s} TalkChannel::SendTextMessageTo[3814396] - I invite you into a wing, you accept the mission, I complete it and you get paid
 *
 * When receiving a message:
 * {13:00:26GMT 3473.688s} TalkChannel:SendTextMessage (received) LuPoN:'If I don't answer, it's because I'm afk at the moment, home duties'
*/
int main(int argc, char** argv) {

    if(argc != 3) {
        cout<<argc<<" arguments"<<endl;
        cout<<"usage: extractor [input folder] [output file]"<<endl;
        return 1;
    }


    ofstream output(argv[2]);

    cout<<"Argument no.: "<<argc<<endl;
    for(int i=0; i<argc; i++) {
        cout<<argv[i]<<endl;
    }

    auto filenames = getFiles(argv[1]);
    char* line = new char[500];

    int idx=1;
    int size = filenames.size();
    // for each .log file, process the contents and add it all to chats.txt
    for(const string& filename : filenames) {
        string date = getFormattedDate(filename);

        // open the file and scan for chat beginnings
        ifstream currentFile(filename);

        int chats = 0;
        string buffer;
        bool empty = true;


        while(!currentFile.eof()) {
            currentFile.getline(line, 500, '\n');
            if(strcmp(line, "") == 0)
                break;
            if(strstr(line, " TalkChannel:") != nullptr and line[0] == '{') {
                // found a chat line, send it for processing
                string result = processChatLine(line);
                buffer += result + "\n";
                empty = false;
                chats++;
            }
        }

        cout<<idx++<<"/"<<size<<" "<<filename<<" Chats: "<<chats<<endl;

        if(empty) {
            continue;
        }
        else {
            output << "{Chat session on: " + date + " beginning}" << endl;
            output << buffer;
            output << "{Chat session on: " + date + " end}" << endl;
        }
    }

    ofstream friend_stats("friend_stats.txt");
    vector<pair<string, int>> all;
    for_each(friends.begin(), friends.end(), [&](std::pair<string, int> entry){
        all.push_back(entry);
    });
    sort(all.begin(), all.end(), [&](pair<string, int> entry1, pair<string, int> entry2){return entry1.second > entry2.second;});

    for(auto entry : all)
        friend_stats<<"Friend: "<<entry.first<<", messages: "<<entry.second<<endl;
    friend_stats.close();

    delete[] line;
    output.close();
    return 0;
}

