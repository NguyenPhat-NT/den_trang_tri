#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
class String {
 std::string value;
 public:
 String(){}String(const char* s):value(s?s:""){}String(const char* s,size_t n):value(s,n){}
 const char* c_str()const{return value.c_str();}size_t length()const{return value.size();}
 bool concat(const char* s){value+=s;return true;}bool concat(const char* s,size_t n){value.append(s,n);return true;}
 String& operator=(const char* s){value=s?s:"";return *this;}bool operator==(const String& s)const{return value==s.value;}bool operator!=(const String& s)const{return value!=s.value;}
};
