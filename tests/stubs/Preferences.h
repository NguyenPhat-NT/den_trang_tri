#pragma once
#include <map>
#include <vector>
#include <string>
#include <cstring>
struct FakeNVS {std::map<std::string,std::vector<uint8_t>> data;bool failWrite=false,torn=false,failOpen=false,corruptWrite=false;};
extern FakeNVS fakeNVS;
class Preferences {
 std::string space;
 public:
 bool begin(const char* s,bool){space=s;return !fakeNVS.failOpen;}
 size_t getBytesLength(const char* key){auto found=fakeNVS.data.find(space+"/"+key);return found==fakeNVS.data.end()?0:found->second.size();}
 size_t getBytes(const char* key,void* p,size_t n){auto found=fakeNVS.data.find(space+"/"+key);if(found==fakeNVS.data.end())return 0;const auto& bytes=found->second;if(n<bytes.size())return 0;if(!bytes.empty())memcpy(p,bytes.data(),bytes.size());return bytes.size();}
 size_t putBytes(const char* key,const void* p,size_t n){if(fakeNVS.failWrite)return 0;const auto* b=static_cast<const uint8_t*>(p);if(fakeNVS.torn){fakeNVS.data[space+"/"+key]={b,b+n/2};return n/2;}auto& stored=fakeNVS.data[space+"/"+key];stored={b,b+n};if(fakeNVS.corruptWrite&&!stored.empty())stored.back()^=1;return n;}
};
