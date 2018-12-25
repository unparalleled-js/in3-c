#include "bytes.h"
#include "utils.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "data.h"
#include "mem.h"
#include "stringbuilder.h"

// len>0 = bytes or string
// len = 0  -> NULL
// len = -1 -> boolean true 
// len = -2 -> boolean false
// len = -3  -> byte

d_key_t key(char* c) {
    uint16_t val=0;
    while (true) {
        if (*c==0) return val;
        val^=*c | val<<7;
        c+=1;
    }
    return val;
}
d_key_t keyn(char* c, int len) {
    uint16_t val=0;
    int i=0;
    for (;i<len;i++) {
        if (*c==0) return val;
        val^=*c | val<<7;
        c+=1;
    }
    return val;
}

bytes_t* d_bytes(d_token_t* item) {
    return (bytes_t*) item;
}
char* d_string(d_token_t* item) {
    if (item==NULL) return NULL;
    return (char*)item->data;
}
int d_int(d_token_t* item) {
    return d_intd(item,0);
}
int d_intd(d_token_t* item, int def_val) {
    if (item==NULL) return def_val;
    switch (d_type(item)) {
        case T_INTEGER:
        case T_BOOLEAN:
          return item->len & 0xFFFFFFF;
        default:
          return def_val;
    }
}

bytes_t ** d_create_bytes_vec(d_token_t* arr) {
    if (arr==NULL) return NULL;
    int l = d_len(arr),i;
    bytes_t ** dst = _calloc(l+1,sizeof(bytes_t*)) ;
    d_token_t* t = arr+1;
    for (i=0;i<l;i++, t+=d_token_size(t)) 
       dst[i]=d_bytes(t); 
    return dst;
}

uint64_t d_long(d_token_t* item) {
    return d_longd(item,0L);
}
uint64_t d_longd(d_token_t* item, uint64_t def_val) {
    if (item==NULL) return def_val;
    if (d_type(item)==T_INTEGER)
      return item->len & 0xFFFFFFF;
    else if (d_type(item) == T_BYTES) {
        return 0;
    }
    return 0;    
}



d_type_t d_type(d_token_t* item) {
    return item==NULL ? T_NULL :(item->len & 0xF0000000)>>28;
}
int d_len(d_token_t* item) {
    if (item==NULL) return 0;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          return item->len & 0xFFFFFFF;
        default:
          return 0;
    }
}

int d_token_size(d_token_t* item) {
    if (item==NULL) return 0;
    int i,c=1;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          for (i=0;i<(item->len & 0xFFFFFFF);i++)
            c+=d_token_size(item+c);
          return c;
        default:
          return 1;
    }
}

d_token_t*  d_get(d_token_t* item, uint16_t key) {
  if (item==NULL) return NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_token_size(item)) {
      if (item->key==key) return item;
  }
  return NULL;
}
d_token_t*  d_get_or(d_token_t* item, uint16_t key, uint16_t key2) {
  if (item==NULL) return NULL;
  d_token_t* s=NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_token_size(item)) {
      if (item->key==key) return item;
      if (item->key==key2) s=item;
  }
  return s;
}

d_token_t*  d_get_at(d_token_t* item, int index) {
  if (item==NULL) return NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_token_size(item)) {
      if (i==index) return item;
  }
  return NULL;
}

d_token_t*  d_next(d_token_t* item) {
   return item==NULL ? NULL : item+d_token_size(item);
}


char next_char(json_parsed_t* jp) {
    while (true) {
        switch (*jp->c) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
              jp->c++;
              break;
            default:
              return *(jp->c++);
        }
    }
}

d_token_t* parsed_next_item(json_parsed_t* jp, d_type_t type, d_key_t key,int parent) {
  if (jp->len+1>jp->allocated) {
      jp->items = _realloc(jp->items,(jp->allocated<<1)*sizeof(d_token_t),jp->allocated*sizeof(d_token_t));
      jp->allocated<<=1;
  }
  d_token_t* n = jp->items+jp->len;
  jp->len+=1;
  n->key=key;
  n->data=NULL;
  n->len=type<<28;
  if (parent>=0) jp->items[parent].len++;
  return n;
}

int parse_key(json_parsed_t* jp) {
    char* start=jp->c;
    int r;
    while (true) {
        switch (*(jp->c++)) {
            case 0: return -2;
            case '"':
              r = keyn(start,jp->c-start-1);
              return next_char(jp)==':' ? r : -2;
            case '\\' :
               jp->c++; 
               break;
        }
    }
}

int parse_number(json_parsed_t* jp, d_token_t* item) {
    char temp[20];
    int i=0;
    jp->c--;
    char* start=jp->c;
    for (;i<20;i++) {
        if (jp->c[i]>='0' && jp->c[i]<='9') 
           temp[i]=jp->c[i];
        else {
            temp[i]=0;
            jp->c+=i;
            item->len |= atoi(temp);
            return 0;
        }
    }
    return -2;
}

int parse_string(json_parsed_t* jp, d_token_t* item) {
    char* start=jp->c;
    int l,i;
    while (true) {
        switch (*(jp->c++)) {
            case 0: return -2;
            case '"': 
               l = jp->c-start-1;
               if (l>1 && *start=='0' && start[1]=='x') {
                   // this is a hex-value
                   if (l<10) { // we can accept up to 3,4 bytes as integer 
                     item->len = T_INTEGER << 28;
                     for (i=2;i<l;i++) 
                       item->len |= strtohex(start[i]) << (l-i-1)*4;
                   }
                   else {
                       // we need to allocate bytes for it. and so set the type to bytes
                       item->len =(l%2?l-1:l-2)/2;
                       item->data=_malloc(item->len);
                       if (l%2)  item->data[0]=strtohex(start[2]);
                       l=l%2+2;
                       for (i=l-2;i<item->len;i++) 
                          item->data[i] = strtohex(start[l+i*2])<<4 | strtohex(start[l+i*2+1]) ;
                   }
               }
               else {
                   item->len = l | T_STRING <<28;
                   item->data = (uint8_t*) start;
                   item->data = _malloc(l+1);
                   memcpy(item->data,start,l);
                   item->data[l]=0;
               }
               return 0; 
            case '\\' : jp->c++; break;
        }
    }
}


int parse_object(json_parsed_t* jp, int parent, uint32_t key) {
    int res, p_index=jp->len;
    

    switch (next_char(jp)) {
        case 0: return -2;
        case '{':
          parsed_next_item(jp,T_OBJECT,key,parent)->data = (uint8_t*) jp->c-1;
          while (true) {
            switch (next_char(jp)) {
                case '"': 
                   res = parse_key(jp);
                   if (res<0) return res;
                   break;
                case '}':  return 0;
                default:   return -2;  // invalid character or end
            }
            res = parse_object(jp,p_index,res);  // parse the value
            if (res<0) return res;
            switch (next_char(jp)) {
                case ',': break;     // we continue reading the next property
                case '}': return 0;  // this was the last property, so we return successfully.
                default : return -2; // unexpected character, throw.
            }
          }
        case '[':
          parsed_next_item(jp,T_ARRAY,key,parent)->data = (uint8_t*) jp->c-1;
          if (next_char(jp)==']') return 0;
          jp->c--;

          while (true) {
            res = parse_object(jp,p_index, jp->items[p_index].len & 0xFFFFFF);  // parse the value
            if (res<0) return res;
            switch (next_char(jp)) {
                case ',': break;     // we continue reading the next property
                case ']': return 0;  // this was the last element, so we return successfully.
                default : return -2; // unexpected character, throw.
            }
          }
        case '"':
          return parse_string(jp,parsed_next_item(jp,T_STRING,key,parent));
        case 't':
          if (strncmp(jp->c,"rue",3)==0) {
              parsed_next_item(jp,T_BOOLEAN,key,parent)->len|=1;
              jp->c+=3;
              return 0;
          }
          else return -2;
        case 'f':
          if (strncmp(jp->c,"alse",4)==0) {
              parsed_next_item(jp,T_BOOLEAN,key,parent);
              jp->c+=4;
              return 0;
          }
          else return -2;
        case 'n':
          if (strncmp(jp->c,"ull",3)==0) {
              parsed_next_item(jp,T_NULL,key,parent);
              jp->c+=3;
              return 0;
          }
          else return -2;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
           return parse_number(jp,parsed_next_item(jp,T_INTEGER,key,parent) );
        default:
           return -2;
    }
 

}

void free_json(json_parsed_t* jp) {
    if (jp->items==NULL) return;
    int i;
    for (i=0;i<jp->len;i++) {
        if (jp->items[i].data!=NULL && d_type(jp->items+i)<2) 
            _free(jp->items[i].data);
    }
    _free(jp->items);
    _free(jp);
}

json_parsed_t* parse_json(char* js ) {
    json_parsed_t* parser=_malloc(sizeof(json_parsed_t));
    parser->len = 0;
    parser->items = _malloc(sizeof(d_token_t)*10);
    parser->c = js;
    parser->allocated = 10;
    int res = parse_object(parser,-1,0);
    if (res<0) {
      free_json(parser);
      return NULL;
    }
    parser->c=js;
    return parser;
}



static int find_end(char* str) {
    int l=0;
    char* c=str;
    while (*c!=0) {
        switch (*(c++)) {
            case '{': 
            case '[': 
               l++; break;
            case '}': 
            case ']': 
               l--; break;
        }
        if (l==0) 
           return c - str;
    }
    return c - str;
}

char* d_create_json(d_token_t* item) {
    char* dst;
    int i,l=d_len(item);
    str_range_t s;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          s= d_to_json(item);
          dst = _malloc(s.len+1);
          memcpy(dst,s.data,s.len);
          dst[s.len]=0;
          return dst;
        case T_BOOLEAN:
          return d_int(item) ? _strdup("true",4) : _strdup("false",5);
        case T_INTEGER:
          dst = _malloc(16);
          sprintf(dst,"0x%x",d_int(item));
          return dst;
        case T_NULL:
          return _strdup("null",4);
        case T_STRING:
          dst = _malloc(l+3);
          dst[0]='"';
          dst[l+1]='"';
          dst[l+2]=0;
          memcpy(dst+1,item->data,l);
          return dst;
        case T_BYTES:
          dst = _malloc(l*2+5);
          dst[0]='"';
          dst[1]='0';
          dst[2]='x';
          int8_to_char(item->data,item->len,dst+3);
          dst[l*2+3]='"';
          dst[l*2+4]=0;
          return dst;
    }
}

str_range_t d_to_json(d_token_t* item) {
    str_range_t s;
    s.data =  (char*) item->data;
    s.len = find_end(s.data);
    return s;
}
