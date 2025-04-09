#include <iostream>

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"

class FeedbackOutputFormat
{
public:
    static void showFeedbackOutput(std::string feedbackType, std::string message)
    {
        if (feedbackType == "error")
        {
            std::cerr << RED;
        }
        else if (feedbackType == "warning")
        {
            std::cerr << YELLOW;
        }
        std::cerr << feedbackType << ": " << RESET << message << std::endl;
    }
};