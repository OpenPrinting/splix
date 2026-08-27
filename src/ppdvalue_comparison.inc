PPDFile::Value::operator double() const {
    double val = 0.0;
    if (!_out.empty()) {
        try {
            val = std::stod(_out);
        } catch (...) {}
    }
    return val;
}

PPDFile::Value::operator long double() const {
    long double val = 0.0;
    if (!_out.empty()) {
        try {
            val = std::stold(_out);
        } catch (...) {}
    }
    return val;
}

bool PPDFile::Value::operator == (std::string_view val) const
{
    if (_out.empty())
        return false;
    return std::ranges::equal(_out, val, [](char c1, char c2) {
        return std::tolower(static_cast<unsigned char>(c1)) ==
               std::tolower(static_cast<unsigned char>(c2));
    });
}

bool PPDFile::Value::operator == (const char *val) const
{
    if (_out.empty() || !val)
        return false;
    return (*this) == std::string_view(val);
}

bool PPDFile::Value::operator != (std::string_view val) const
{
    return !((*this) == val);
}

bool PPDFile::Value::operator != (const char *val) const
{
    return !((*this) == val);
}



/*
 * Copy constructor
 */
PPDFile::Value::Value(const PPDFile::Value &val)
{
    _raw = val._raw;
    _preformatted = val._preformatted;
    _out = val._preformatted.empty() ? _raw : _preformatted;
    _hasValue = val._hasValue;
    _width = val._width;
    _height = val._height;
    _marginX = val._marginX;
    _marginY = val._marginY;
}

PPDFile::Value& PPDFile::Value::operator = (const PPDFile::Value &val)
{
    if (this == &val)
        return *this;
    _raw = val._raw;
    _preformatted = val._preformatted;
    _out = val._preformatted.empty() ? _raw : _preformatted;
    _hasValue = val._hasValue;
    _width = val._width;
    _height = val._height;
    _marginX = val._marginX;
    _marginY = val._marginY;
    return *this;
}


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
