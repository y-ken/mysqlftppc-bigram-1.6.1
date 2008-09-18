#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ftbool.h"
#if HAVE_ICU
#include "ftnorm.h"
#endif

// mysql headers
#include <my_global.h>
#include <m_ctype.h>
#include <my_sys.h>
#include <plugin.h>

#if !defined(__attribute__) && (defined(__cplusplus) || !defined(__GNUC__)  || __GNUC__ == 2 && __GNUC_MINOR__ < 8)
#define __attribute__(A)
#endif

static char* bigram_unicode_normalize;
static char* bigram_collation;

static int bigram_parser_plugin_init(void *arg __attribute__((unused)))
{
  return(0);
}
static int bigram_parser_plugin_deinit(void *arg __attribute__((unused)))
{
  return(0);
}


static int bigram_parser_init(MYSQL_FTPARSER_PARAM *param __attribute__((unused)))
{
  return(0);
}
static int bigram_parser_deinit(MYSQL_FTPARSER_PARAM *param __attribute__((unused)))
{
  return(0);
}

static size_t str_convert(CHARSET_INFO *cs, char *from, int from_length,
                          CHARSET_INFO *uc, char *to,   int to_length){
  char *rpos, *rend, *wpos, *wend;
  my_charset_conv_mb_wc mb_wc = cs->cset->mb_wc;
  my_charset_conv_wc_mb wc_mb = uc->cset->wc_mb;
  my_wc_t wc;
  
  rpos = from;
  rend = from + from_length;
  wpos = to;
  wend = to + to_length;
  while(rpos < rend){
    int cnvres = 0;
    cnvres = (*mb_wc)(cs, &wc, (uchar*)rpos, (uchar*)rend);
    if(cnvres > 0){
      rpos += cnvres;
    }else if(cnvres == MY_CS_ILSEQ){
      rpos++;
      wc = '?';
    }else if(cnvres > MY_CS_TOOSMALL){
      rpos += (-cnvres);
      wc = '?';
    }else{
      break;
    }
    cnvres = (*wc_mb)(uc, wc, (uchar*)wpos, (uchar*)wend);
    if(cnvres > 0){
      wpos += cnvres;
    }else{
      break;
    }
  }
  return (size_t)(wpos - to);
}

static int bigram_parser_parse(MYSQL_FTPARSER_PARAM *param)
{
  CHARSET_INFO *uc = NULL; // if the sequence changed to UTF-8, it is not null
  CHARSET_INFO *collator = NULL; // if we use collation, it is not null
  CHARSET_INFO *cs = param->cs;
  char* feed = param->doc;
  size_t feed_length = (size_t)param->length;
  
  size_t mblen;
  char* cv;
  size_t cv_length=0;
  
  if(strcmp("utf8", cs->csname)!=0 && strcmp(bigram_unicode_normalize, "OFF")!=0){
    uc = get_charset(33,MYF(0)); // my_charset_utf8_general_ci for utf8 conversion
  }
  
  // convert into UTF-8
  if(uc){
    // calculate mblen and malloc.
    mblen = uc->mbmaxlen * cs->cset->numchars(cs, feed, feed+feed_length);
    cv = my_malloc(mblen, MYF(MY_WME));
    cv_length = mblen;
    feed_length = str_convert(cs, feed, feed_length, uc, cv, cv_length);
    feed = cv;
  }
  
#if HAVE_ICU
  // normalize
  if(strcmp(bigram_unicode_normalize, "OFF")!=0){
    char* nm;
    size_t nm_length=0;
    size_t nm_used=0;
    nm_length = feed_length+32;
    nm = my_malloc(nm_length, MYF(MY_WME));
    int mode = 1;
    if(strcmp(bigram_unicode_normalize, "C")==0) mode = 4;
    if(strcmp(bigram_unicode_normalize, "D")==0) mode = 2;
    if(strcmp(bigram_unicode_normalize, "KC")==0) mode = 5;
    if(strcmp(bigram_unicode_normalize, "KD")==0) mode = 3;
    if(strcmp(bigram_unicode_normalize, "FCD")==0) mode = 6;
    if(uni_normalize(feed, feed_length, nm, nm_length, &nm_used, mode)!=0){
       nm_length=nm_used;
       nm = my_realloc(nm, nm_length, MYF(MY_WME));
       uni_normalize(feed, feed_length, nm, nm_length, &nm_used, mode);
    }
    if(cv_length){
      cv = my_realloc(cv, nm_used, MYF(MY_WME));
    }else{
      cv = my_malloc(nm_used, MYF(MY_WME));
    }
    memcpy(cv, nm, nm_used);
    cv_length = nm_used;
    my_free(nm, MYF(0));
    feed = cv;
    feed_length = cv_length;
  }
#endif
  
  if(strcmp(bigram_collation, "no")!=0){
//     bin 90
//     czech 138
//     roman 143
//     danish 139
//     slovak 141
//     polish 133
//     spanish 135
//     swedish 136
//     turkish 137
//     general 35
//     unicode 128
//     latvian 130
//     persian 144
//     spanish2 142
//     romanian 131
//     estonian 134
//     icelandic 129
//     slovenian 132
//     esperanto 145
//     hungarian 146
//     lithuanian 140
    // charset is always UCS2
    if(strcmp(bigram_collation, "bin")==0) collator = get_charset(90,MYF(0));
    if(strcmp(bigram_collation, "czech")==0) collator = get_charset(138,MYF(0));
    if(strcmp(bigram_collation, "roman")==0) collator = get_charset(143,MYF(0));
    if(strcmp(bigram_collation, "danish")==0) collator = get_charset(139,MYF(0));
    if(strcmp(bigram_collation, "slovak")==0) collator = get_charset(141,MYF(0));
    if(strcmp(bigram_collation, "polish")==0) collator = get_charset(133,MYF(0));
    if(strcmp(bigram_collation, "spanish")==0) collator = get_charset(135,MYF(0));
    if(strcmp(bigram_collation, "swedish")==0) collator = get_charset(136,MYF(0));
    if(strcmp(bigram_collation, "turkish")==0) collator = get_charset(137,MYF(0));
    if(strcmp(bigram_collation, "general")==0) collator = get_charset(35,MYF(0));
    if(strcmp(bigram_collation, "unicode")==0) collator = get_charset(128,MYF(0));
    if(strcmp(bigram_collation, "latvian")==0) collator = get_charset(130,MYF(0));
    if(strcmp(bigram_collation, "persian")==0) collator = get_charset(144,MYF(0));
    if(strcmp(bigram_collation, "spanish2")==0) collator = get_charset(142,MYF(0));
    if(strcmp(bigram_collation, "romanian")==0) collator = get_charset(131,MYF(0));
    if(strcmp(bigram_collation, "estonian")==0) collator = get_charset(134,MYF(0));
    if(strcmp(bigram_collation, "icelandic")==0) collator = get_charset(129,MYF(0));
    if(strcmp(bigram_collation, "slovenian")==0) collator = get_charset(132,MYF(0));
    if(strcmp(bigram_collation, "esperanto")==0) collator = get_charset(145,MYF(0));
    if(strcmp(bigram_collation, "hungarian")==0) collator = get_charset(146,MYF(0));
    if(strcmp(bigram_collation, "lithuanian")==0) collator = get_charset(140,MYF(0));
  }
  
//  char *rpos, *docend= param->doc + param->length;
//  uchar ustr[2];
  
  // setup params
  int qmode = param->mode;
  param->flags = MYSQL_FTFLAGS_NEED_COPY; // buffer is to be free-ed
  MYSQL_FTPARSER_BOOLEAN_INFO bool_info_may ={ FT_TOKEN_WORD, 0, 0, 0, 0, ' ', 0 };
  
  int context=CTX_CONTROL;
  int depth=0;
  MYSQL_FTPARSER_BOOLEAN_INFO instinfo;
  MYSQL_FTPARSER_BOOLEAN_INFO baseinfos[16];
  instinfo = baseinfos[0] = bool_info_may;
  
  // working buffers:
  uchar   gram_buffer[6];
  my_wc_t gram_wc[3];
  uchar  ustr[2];
  // working buffer to hold partial weighting vector
  uchar* w_buffer;
  size_t w_buffer_len=0;
  // malloc working space.
  if(collator){ // fetch from UTF-8 sequence -> put on weighting vector on working space
    w_buffer_len = collator->coll->strnxfrmlen(collator, 1);
    w_buffer     = (uchar*)my_malloc(w_buffer_len, MYF(MY_WME));
  }else{ // wc -> mb conversion
    w_buffer_len = sizeof(uchar) * cs->mbmaxlen * 2;
    w_buffer     = (uchar*)my_malloc(w_buffer_len, MYF(MY_WME));
  }
  size_t wpos=0;  // scanning position of weighting sub-vector.
  size_t wlen=0; // the length of the sub-vector.
  
  int ct=0;
  char* pos = feed;
  char* docend = feed + feed_length;
  while(pos < docend){
    int convres;
    my_wc_t wc;
    if(collator && wpos < wlen){
      // we can use that weight
    }else{
      int broken=1;
      while(pos < docend){
        SEQFLOW sf;
        if(uc){
          sf = ctxscan(uc, pos, docend, &wc, &convres, context);
        }else{
          sf = ctxscan(cs, pos, docend, &wc, &convres, context);
        }
        if(convres > 0){
          pos += convres;
        }else if(convres == MY_CS_ILSEQ){
          pos++;
          wc = '?';
        }else if(convres > MY_CS_TOOSMALL){
          pos += (-convres);
          wc = '?';
        }else{
          break;
        }
        
        if(qmode == MYSQL_FTPARSER_FULL_BOOLEAN_INFO){
          if(collator && sf==SF_TRUNC) sf=SF_CHAR; // trunc will not be supported with collator
          
          if(sf==SF_ESCAPE){
            context |= CTX_ESCAPE;  // ESCAPE ON
            context |= CTX_CONTROL; // CONTROL ON
          }else{
            context &= ~CTX_ESCAPE; // ESCAPE OFF
            if(sf == SF_CHAR){
              context &= ~CTX_CONTROL; // CONTROL OFF
            }else{
              context |= CTX_CONTROL; // CONTROL ON
            }
            
            if(sf == SF_QUOTE_START) context |= CTX_QUOTE;
            if(sf == SF_QUOTE_END)   context &= ~CTX_QUOTE;
            if(sf == SF_LEFT_PAREN){
              instinfo = baseinfos[depth];
              depth++;
              if(depth>16) depth=16;
              baseinfos[depth] = instinfo;
              instinfo.type = FT_TOKEN_LEFT_PAREN;
              param->mysql_add_word(param, pos, 0, &instinfo); // push LEFT_PAREN token
            }
            if(sf == SF_RIGHT_PAREN){
              instinfo.type = FT_TOKEN_RIGHT_PAREN;
              param->mysql_add_word(param, pos, 0, &instinfo); // push RIGHT_PAREN token
              depth--;
              if(depth<0) depth=0;
            }
            if(sf == SF_PLUS){
              instinfo.yesno = 1;
            }
            if(sf == SF_MINUS){
              instinfo.yesno = -1;
            }
            if(sf == SF_PLUS) instinfo.weight_adjust = 1;
            if(sf == SF_MINUS) instinfo.weight_adjust = -1;
            if(sf == SF_WASIGN){
              instinfo.wasign = -1;
            }
          }
          if(sf == SF_WHITE || sf == SF_QUOTE_END || sf == SF_LEFT_PAREN || sf == SF_RIGHT_PAREN){
            instinfo = baseinfos[depth];
          }
          if(sf == SF_CHAR){
            broken = 0;
            break; // emit char
          }else if(sf != SF_ESCAPE){
            param->mode = MYSQL_FTPARSER_FULL_BOOLEAN_INFO; // reset phrase query
            ct=0;
          }
        }else{
          broken = 0;
          break; // emit char
        }
      }
      if(broken) break;
      
      if(collator){
        convres = collator->cset->wc_mb(collator, wc, ustr, ustr+2);
        if(convres <= 0) break;
        wlen=collator->coll->strnxfrm(collator, w_buffer, w_buffer_len, ustr, (size_t)2);
        // trim() because mysql binary image has padding.
        int c,mark;
        uint t_res= collator->sort_order_big[0][0x20 * collator->sort_order[0]];
        for(mark=0,c=0; c<wlen; c+=2){
            if((*(w_buffer+c) == (t_res>>8)) && (*(w_buffer+c+1) == (t_res&0xFF))){
              // it is space or padding.
            }else{
              mark = c;
            }
        }
        wlen = mark+2;
        wpos = 0;
      }
    }
    
    if(ct%2==0){
      if(collator){
          gram_buffer[0] = w_buffer[wpos];
          gram_buffer[1] = w_buffer[wpos+1];
          gram_buffer[4] = w_buffer[wpos];
          gram_buffer[5] = w_buffer[wpos+1];
      }else{
        gram_wc[0] = wc;
        gram_wc[2] = wc;
      }
      if(ct!=0){ // skip the first loop. we got only one char at that time.
        if(qmode == MYSQL_FTPARSER_FULL_BOOLEAN_INFO){
          if(collator){
            param->mysql_add_word(param, (char*)(gram_buffer+2), 4, &instinfo);
          }else{
            convres = cs->cset->wc_mb(cs, gram_wc[1], w_buffer, w_buffer+w_buffer_len);
            convres = cs->cset->wc_mb(cs, gram_wc[2], w_buffer+convres, w_buffer+w_buffer_len) + convres;
            if(convres>0) param->mysql_add_word(param, (char*)w_buffer, convres, &instinfo);
          }
          param->mode = MYSQL_FTPARSER_WITH_STOPWORDS;
        }else{
          if(collator){
            param->mysql_add_word(param, (char*)(gram_buffer+2), 4, NULL);
          }else{
            convres = cs->cset->wc_mb(cs, gram_wc[1], w_buffer, w_buffer+w_buffer_len);
            convres = cs->cset->wc_mb(cs, gram_wc[2], w_buffer+convres, w_buffer+w_buffer_len) + convres;
            if(convres>0) param->mysql_add_word(param, (char*)w_buffer, convres, NULL);
          }
        }
      }
    }else{
      if(collator){
        gram_buffer[2] = w_buffer[wpos];
        gram_buffer[3] = w_buffer[wpos+1];
      }else{
        gram_wc[1] = wc;
      }
      if(qmode == MYSQL_FTPARSER_FULL_BOOLEAN_INFO){
        if(collator){
          param->mysql_add_word(param, (char*)gram_buffer, 4, &instinfo);
        }else{
          convres = cs->cset->wc_mb(cs, gram_wc[0], w_buffer, w_buffer+w_buffer_len);
          convres = cs->cset->wc_mb(cs, gram_wc[1], w_buffer+convres, w_buffer+w_buffer_len) + convres;
          if(convres>0) param->mysql_add_word(param, (char*)w_buffer, convres, &instinfo);
        }
        param->mode = MYSQL_FTPARSER_WITH_STOPWORDS;
      }else{
        if(collator){
          param->mysql_add_word(param, (char*)gram_buffer, 4, NULL);
        }else{
          convres = cs->cset->wc_mb(cs, gram_wc[0], w_buffer, w_buffer+w_buffer_len);
          convres = cs->cset->wc_mb(cs, gram_wc[1], w_buffer+convres, w_buffer+w_buffer_len) + convres;
          if(convres>0) param->mysql_add_word(param, (char*)w_buffer, convres, NULL);
        }
      }
    }
    ct++;
    wpos+=2;
  }
  
  my_free(w_buffer, MYF(0));
  if(cv_length > 0) my_free(cv, MYF(0));
  return(0);
}

int bigram_unicode_normalize_check(MYSQL_THD thd, struct st_mysql_sys_var *var, void *save, struct st_mysql_value *value){
    char buf[4];
    int len=4;
    const char *str;
    
    str = value->val_str(value,buf,&len);
    if(!str) return -1;
    *(const char**)save=str;
    if(!get_charset(33,MYF(0))) return -1; // If you don't have utf8 codec in mysql, it fails
    if(len==1){
        if(str[0]=='C'){ return 0;}
        if(str[0]=='D'){ return 0;}
    }
    if(len==2){
        if(str[0]=='K' && str[1]=='C'){ return 0;}
        if(str[0]=='K' && str[1]=='D'){ return 0;}
    }
    if(len==3){
        if(str[0]=='F' && str[1]=='C' && str[2]=='D'){ return 0;}
        if(str[0]=='O' && str[1]=='F' && str[2]=='F'){ return 0;}
    }
    return -1;
}
int bigram_collation_check(MYSQL_THD thd, struct st_mysql_sys_var *var, void *save, struct st_mysql_value *value){
    char buf[32];
    int len=32;
    const char *str;
    
    str = value->val_str(value,buf,&len);
    if(!str) return -1;
    *(const char**)save = str;
//     bin 90
//     czech 138
//     roman 143
//     danish 139
//     slovak 141
//     polish 133
//     spanish 135
//     swedish 136
//     turkish 137
//     general 35
//     unicode 128
//     latvian 130
//     persian 144
//     spanish2 142
//     romanian 131
//     estonian 134
//     icelandic 129
//     slovenian 132
//     esperanto 145
//     hungarian 146
//     lithuanian 140
    if(len==2){
        if(memcmp("no", str, len)==0) return 0;
    }
    if(len==3){
        if(memcmp("bin", str, len)==0 && get_charset(90, MYF(0))!=NULL) return 0;
    }
    if(len==5){
        if(memcmp("czech", str, len)==0 && get_charset(138, MYF(0))!=NULL) return 0;
        if(memcmp("roman", str, len)==0 && get_charset(143, MYF(0))!=NULL) return 0;
    }
    if(len==6){
        if(memcmp("danish", str, len)==0 && get_charset(139, MYF(0))!=NULL) return 0;
        if(memcmp("slovak", str, len)==0 && get_charset(141, MYF(0))!=NULL) return 0;
        if(memcmp("polish", str, len)==0 && get_charset(133, MYF(0))!=NULL) return 0;
    }
    if(len==7){
        if(memcmp("spanish", str, len)==0 && get_charset(135, MYF(0))!=NULL) return 0;
        if(memcmp("swedish", str, len)==0 && get_charset(136, MYF(0))!=NULL) return 0;
        if(memcmp("turkish", str, len)==0 && get_charset(137, MYF(0))!=NULL) return 0;
        if(memcmp("general", str, len)==0 && get_charset( 35, MYF(0))!=NULL) return 0;
        if(memcmp("unicode", str, len)==0 && get_charset(128, MYF(0))!=NULL) return 0;
        if(memcmp("latvian", str, len)==0 && get_charset(130, MYF(0))!=NULL) return 0;
        if(memcmp("persian", str, len)==0 && get_charset(144, MYF(0))!=NULL) return 0;
    }
    if(len==8){
        if(memcmp("spanish2", str, len)==0 && get_charset(142, MYF(0))!=NULL) return 0;
        if(memcmp("romanian", str, len)==0 && get_charset(131, MYF(0))!=NULL) return 0;
        if(memcmp("estonian", str, len)==0 && get_charset(134, MYF(0))!=NULL) return 0;
    }
    if(len==9){
        if(memcmp("icelandic", str, len)==0 && get_charset(129, MYF(0))!=NULL) return 0;
        if(memcmp("slovenian", str, len)==0 && get_charset(132, MYF(0))!=NULL) return 0;
        if(memcmp("esperanto", str, len)==0 && get_charset(145, MYF(0))!=NULL) return 0;
        if(memcmp("hungarian", str, len)==0 && get_charset(146, MYF(0))!=NULL) return 0;
    }
    if(len==10){
        if(memcmp("lithuanian", str, len)==0 && get_charset(140, MYF(0))!=NULL) return 0;
    }
    return -1;
}
static MYSQL_SYSVAR_STR(normalization, bigram_unicode_normalize,
  PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC,
  "Set unicode normalization (OFF, C, D, KC, KD, FCD)",
  bigram_unicode_normalize_check, NULL, "OFF");

static MYSQL_SYSVAR_STR(collation, bigram_collation,
  PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC,
  "Set unicode collation mode (OFF, unicode, general, etc.)",
  bigram_collation_check, NULL, "OFF");

static struct st_mysql_sys_var* bigram_system_variables[]= {
  MYSQL_SYSVAR(collation),
#if HAVE_ICU
  MYSQL_SYSVAR(normalization),
#endif
  NULL
};

static struct st_mysql_ftparser bigram_parser_descriptor=
{
  MYSQL_FTPARSER_INTERFACE_VERSION, /* interface version      */
  bigram_parser_parse,              /* parsing function       */
  bigram_parser_init,               /* parser init function   */
  bigram_parser_deinit              /* parser deinit function */
};

mysql_declare_plugin(ft_bigram)
{
  MYSQL_FTPARSER_PLUGIN,      /* type                            */
  &bigram_parser_descriptor,  /* descriptor                      */
  "bigram",                   /* name                            */
  "Hiroaki Kawai",            /* author                          */
  "Bi-gram Full-Text Parser", /* description                     */
  PLUGIN_LICENSE_BSD,
  bigram_parser_plugin_init,  /* init function (when loaded)     */
  bigram_parser_plugin_deinit,/* deinit function (when unloaded) */
  0x0010,                     /* version                         */
  NULL,                       /* status variables                */
  bigram_system_variables,    /* system variables                */
  NULL
}
mysql_declare_plugin_end;

