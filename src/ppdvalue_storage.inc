PPDFile::Value::Value() :
    _width(0.0f), _height(0.0f), _marginX(0.0f), _marginY(0.0f)
{
}

PPDFile::Value::Value(std::string_view value) :
    _raw(value), _width(0.0f), _height(0.0f), _marginX(0.0f),
    _marginY(0.0f), _out(_raw), _hasValue(true)
{
}

PPDFile::Value::~Value() = default;

PPDFile::Value& PPDFile::Value::set(std::string_view value)
{
    _preformatted.clear();
    _raw = value;
    _out = _raw;
    _hasValue = true;

    return *this;
}

PPDFile::Value& PPDFile::Value::set(float width, float height, float marginX,
    float marginY)
{
    _width = width;
    _height = height;
    _marginX = marginX;
    _marginY = marginY;

    return *this;
}

PPDFile::Value& PPDFile::Value::setPreformatted()
{
    if (_raw.empty())
        return *this;

    const char *str = _raw.c_str();

    _preformatted.clear();
    _preformatted.reserve(_raw.size());

    while (*str) {
        if (*str == '<' && strlen(str) >= 3 && isxdigit(*(str+1))) {
            char temp[3] = {0, 0, 0};

            str++;
            temp[0] = *str;
            str++;
            if (*str != '>' && (!isxdigit(*str) || *(str + 1) != '>')) {
                _preformatted += '<';
                _preformatted += temp[0];
                continue;
            }
            if (*str != '>') {
                temp[1] = *str;
                str++;
            }
            _preformatted += static_cast<char>(strtol(temp, nullptr, 16));
            if (*str == '>')
                str++;
            continue;
        }
        _preformatted += *str;
        str++;
    }
    _out = _preformatted;

    return *this;
}

std::string PPDFile::Value::deepCopy() const
{
    if (_out.empty())
        return "";
    return std::string(_out);
}

bool PPDFile::Value::isTrue() const
{
    if (_out.empty())
        return false;

    auto compare = [this](std::string_view target) {
        return std::ranges::equal(_out, target, [](char c1, char c2) {
            return std::tolower(static_cast<unsigned char>(c1)) ==
                   std::tolower(static_cast<unsigned char>(c2));
        });
    };

    if (_out == "1" || compare("true") ||
        compare("enable") || compare("enabled") ||
        compare("yes") || compare("on"))
        return true;
    return false;
}

PPDFile::Value::operator unsigned long() const {
    unsigned long val = 0;
    if (!_out.empty()) {
        std::from_chars(_out.data(), _out.data() + _out.size(), val);
    }
    return val;
}

PPDFile::Value::operator long() const {
    long val = 0;
    if (!_out.empty()) {
        std::from_chars(_out.data(), _out.data() + _out.size(), val);
    }
    return val;
}

PPDFile::Value::operator float() const {
    float val = 0.0f;
    if (!_out.empty()) {
        try {
            val = std::stof(_out);
        } catch (...) {}
    }
    return val;
}
