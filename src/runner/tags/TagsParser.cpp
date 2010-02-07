
#include <string>
#include <iostream>

#include <testngpp/runner/AndCompositeTaggableFilter.h>
#include <testngpp/runner/OrCompositeTaggableFilter.h>
#include <testngpp/runner/NotCompositeTaggableFilter.h>
#include <testngpp/runner/GeneralTagsFilter.h>
#include <testngpp/runner/EmptyTagFilter.h>
#include <testngpp/runner/PosixFNMatcher.h>

#include <testngpp/utils/StringList.h>
#include <testngpp/runner/TagsFilters.h>
#include <testngpp/runner/TagsParser.h>

#include <testngpp/Error.h>

TESTNGPP_NS_START

#define TESGNTPP_PARSE_TAGS_ERR() throw Error("Invalid Tags filter option")
namespace 
{

    enum TokenType
    {
        TOKEN_EMPTY,
        TOKEN_NOT,
        TOKEN_LP,
        TOKEN_RP,
        TOKEN_LS,
        TOKEN_RS,
        TOKEN_STAR,
        TOKEN_DOLLAR,
        TOKEN_TAG,
    };
    
    struct Token
    {
        Token(TokenType tokenType)
        : type(tokenType)
        , tag("")
        {}
        
        Token(std::string tagValue)
        : type(TOKEN_TAG)
        , tag(tagValue)
        {}
        
        TokenType type;
        std::string tag;
    };
    
    bool
    isEmptyString(const std::string& tags)
    {
        for(unsigned int i=0; i<tags.size(); i++)
        {
            if(!::isspace(tags[i]))
                return false;
        }
        
        return true;
    }

    struct CharTokenMapEntry
    {
        char c;
        TokenType tokenType;
    };
    
    CharTokenMapEntry directMap[] = 
    {
    {'^', TOKEN_NOT},
    {'[', TOKEN_LS},
    {']', TOKEN_RS},
    {'(', TOKEN_LP},
    {')', TOKEN_RP}
    };
    
    CharTokenMapEntry indirectMap[] = 
    {
    {'*', TOKEN_STAR},
    {'$', TOKEN_DOLLAR},
    };
 
   bool isTokenChar(char c)
   {
      return !::isblank(c) && c != '\0' && c != '[' && c != ']' && c != '(' && c != ')';
   }
   ///////////////////////////////////////////////////////////////
    Token
    parseToken(char** pStr, bool isBegin = false)
    {
       char* p = *pStr;

       if(::isblank(*p))
       {
          while(::isblank(*p)) p++;
       }

       if(*p == '\0')
       {
          (*pStr) = p;
          return Token(TOKEN_EMPTY);
       }
       
       for(unsigned int i=0; i<sizeof(directMap)/sizeof(directMap[0]); i++)
       {
          if(*p == directMap[i].c)
          {
             p++; (*pStr) = p;
             return Token(directMap[i].tokenType);
          }
       }
       

      for(unsigned int i=0; i<sizeof(indirectMap)/sizeof(indirectMap[0]); i++)
      {
         if(*p == indirectMap[i].c && !isTokenChar(*(p+1)))
         {
            p++; (*pStr) = p;
            return Token(indirectMap[i].tokenType);
         }
      }
       
       std::string tag("");

       while(isTokenChar(*p))
       {
          tag.push_back(*p);
          p++;
       }
        
       (*pStr) = p;
        
       return Token(tag);        
    }

   TaggableObjFilter* parseNOT(char** pp);
   TaggableObjFilter* parseOR(char** pp, bool parseInScope=true, bool isStart=false);
   TaggableObjFilter* parseAND(char** pp);
   
   void doParseAND(AndCompositeTaggableFilter* filter, char** pp)
   {
      while(1)
      {
         Token token = parseToken(pp);
         switch (token.type) 
         {
             case TOKEN_NOT:
                filter->addFilter(parseNOT(pp)); 
                break;
             case TOKEN_TAG:
                filter->addFilter(new GeneralTagsFilter(new PosixFNMatcher(token.tag)));
                break;
             case TOKEN_LP:
                filter->addFilter(parseOR(pp));
                break;
             case TOKEN_LS:
                filter->addFilter(parseAND(pp));
                break;
             case TOKEN_RS:
                return; 
             default:
                TESGNTPP_PARSE_TAGS_ERR();
         }
      }
   }
   
   ///////////////////////////////////////////////////////////////
   TaggableObjFilter*
   parseAND(char** pp)
   {
      AndCompositeTaggableFilter * filter = new AndCompositeTaggableFilter();
        
      try
      {
         doParseAND(filter, pp);
      }
      catch (...) 
      {
         delete filter;
         throw;
      }
      
      return filter;
   }
    
   ///////////////////////////////////////////////////////////////
   TaggableObjFilter*
   parseNOT(char** pp)
   {
        TaggableObjFilter* filter = 0;
        
        Token token = parseToken(pp, false);
        switch (token.type) 
        {
        case TOKEN_TAG:
            filter = new GeneralTagsFilter(new PosixFNMatcher(token.tag));
            break;
        case TOKEN_LP:
            filter = parseOR(pp, true);
            break;
        case TOKEN_LS:
            filter = parseAND(pp);
            break;
        case TOKEN_DOLLAR:
            filter = new EmptyTagFilter();
            break;
        default:
            TESGNTPP_PARSE_TAGS_ERR();
        } 
        
        return new NotCompositeTaggableFilter(filter);
    }

   void doParseOR(OrCompositeTaggableFilter * filter, char** pp, bool parseInScope, bool isStart)
   {
      unsigned int numberOfTags = 0;
      bool topGroup = isStart;
      bool hasMatchAll = false;
      
      while(1)
      {
         Token token = parseToken(pp, isStart);
         
         if(hasMatchAll && token.type != TOKEN_EMPTY)
         {
            TESGNTPP_PARSE_TAGS_ERR();
         }
         
         switch (token.type) 
         {
             case TOKEN_NOT:
                filter->addFilter(parseNOT(pp)); 
                break;
             case TOKEN_TAG:
                filter->addFilter(new GeneralTagsFilter(new PosixFNMatcher(token.tag)));
                break;
             case TOKEN_LP:
                filter->addFilter(parseOR(pp));
                break;
             case TOKEN_LS:
                filter->addFilter(parseAND(pp));
                break;
             case TOKEN_DOLLAR:
                filter->addFilter(new EmptyTagFilter());
                break;
             case TOKEN_STAR:
                if(numberOfTags > 0 || !topGroup)
                {
                   TESGNTPP_PARSE_TAGS_ERR();
                }
                else
                {
                   hasMatchAll = true;
                }
                break;
             case TOKEN_RP:
                if(parseInScope && numberOfTags > 0)
                {
                   return; 
                }

                TESGNTPP_PARSE_TAGS_ERR();
             case TOKEN_EMPTY:
                if(!parseInScope && numberOfTags > 0)
                {
                   return; 
                }
                TESGNTPP_PARSE_TAGS_ERR();
             default:
                TESGNTPP_PARSE_TAGS_ERR();
         }
         
         isStart = false;
         numberOfTags++;
      }
   }
   
   ///////////////////////////////////////////////////////////////
   TaggableObjFilter*
   parseOR(char** pp, bool parseInScope, bool isStart)
   {
            
      OrCompositeTaggableFilter * filter = new OrCompositeTaggableFilter();
      
      try 
      {
         doParseOR(filter, pp, parseInScope, isStart);
         if(filter->isEmpty())
         {
            delete filter;
            
            return 0;
         }
      }
      catch (...)
      {
         delete filter;
         throw;
      }
      
      return filter;
    }
    
   ///////////////////////////////////////////////////////////////
   TaggableObjFilter*
   parseFilter(const std::string& tags)
   {       
      if(isEmptyString(tags))
      {
         TESGNTPP_PARSE_TAGS_ERR();
      }
      
      char* p = const_cast<char*>(tags.c_str());
      return parseOR(&p, false, true);
    }
}

//////////////////////////////////////////////////////////////////
void parseSpec(TagsFilters* tagsFilters, const std::string& spec)
{
   if(spec.size() == 0)
   {
      return;
   }
   
   std::string tags;
   std::string rest;
   
   size_t pos = spec.find_first_of('>');
   if(pos == std::string::npos)
   {
      tags = spec;
   }
   else
   {
      tags = spec.substr(0, pos);
      rest = spec.substr(pos+1, spec.size() - pos);
   }   
   
   TaggableObjFilter* filter = parseFilter(tags);
   tagsFilters->addNextFilter(filter);
   
   parseSpec(tagsFilters, rest);
}

//////////////////////////////////////////////////////////////////
TagsFilters* 
TagsParser::parse( const std::string& tagsSpec )
{
   TagsFilters* tagsFilters = new TagsFilters();
 
   try
   {
      parseSpec(tagsFilters, tagsSpec);
   }
   catch (...) 
   {
      delete tagsFilters;
      throw;
   }
   
   
   return tagsFilters;
}

TESTNGPP_NS_END