#ifndef STYLE_H
#define STYLE_H


#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

namespace cman {
    inline namespace v1 {
        typedef enum {
            WARNING,
            INFO,
            DEBUG,
            ERROR,
        }MessageType;


        void print_message(const char* message, MessageType type);
        void print_help();
    }
}

#endif // STYLE_H
