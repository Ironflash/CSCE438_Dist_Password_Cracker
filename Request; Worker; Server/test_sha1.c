/* 
    File: request.c

    Authors: Daniel Timothy S. Tan
             Department of Computer Science
             Texas A&M University
    Date   : 01/30/2013

    request client main program for HW2 in CSCE 438-500
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    string request_msg = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3";

    char buf [256];
    
    system("echo -n test | shasum | awk '{print $1}'");


    FILE *p = popen("echo -n test | shasum | awk '{print $1}'", "r");
    //FILE *p = popen("echo foo", "r");
    string s;
    size_t c = fread(buf, 1, sizeof(buf), p);
    //cout<<"size = "<<c<<endl;
    s += string(buf, buf + c);
    cout<<"s = "<<s<<endl;

    for (size_t count; (count = fread(buf, 1, sizeof(buf), p));)
        s += string(buf, buf + count);
    pclose(p);
    //*/

    /*
    FILE *p = popen("echo hello", "r");
    sprintf(buf, "%d", fileno(p));
    ifstream p2(buf);
    string s;
    p2 >> s;
    p2.close();
    pclose(p);
    */

    /*
    stringstream buffer;
    streambuf * old = cout.rdbuf(buffer.rdbuf());
    //std::cout << "Bla" << std::endl;
    system("echo foo");
    string text = buffer.str(); // text will now contain "Bla\n"
    */

    cout<<endl<<"Found: "<<s<<endl;
    // compare:
    if (s == request_msg) {
        //cout<<"THEY ARE THE SAME! =D"<<endl;
    } else {
        //cout<<"they are not the same =("<<endl;
    }

    //cout<<"return"<<ret<<endl;
    //fprintf(stdout,"system ret:[%d]\n",ret>>8);
    //system("echo -n test | shasum");
    return 0;
}
