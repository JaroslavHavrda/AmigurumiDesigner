export module prescription_parser;
import std;

export struct prescription_parser
{
    std::vector<float> parsed_numbers{ 1 };
    std::string error;
    void update_prescription(const std::string_view& prescription)
    {
        using std::operator""sv;
        using std::operator""s;
        std::vector<float> prescriptions_num;
        constexpr auto delim{ ","sv };
        for (const auto& item : std::views::split(prescription, "\n"sv))
        {
            int parsed_number = 1;
            std::string sv{ item.begin(), item.end() };
            try {
                parsed_number = std::stoi(sv);
            }
            catch (std::invalid_argument const&)
            {
                error = "syntax_error"s + std::string(item.begin(), item.end());
                return;
            }
            catch (std::out_of_range const&)
            {
                error = "¨too big number:"s + std::string(item.begin(), item.end());
                return;
            }
            prescriptions_num.push_back((float)parsed_number);

        }
        if (prescriptions_num.empty())
        {
            error = "the prescription is empty";
            return;
        }
        parsed_numbers = prescriptions_num;
        error = "";
    }
};
