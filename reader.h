#include "mysqldep.h"

#define FT_EOS 0xFFFF

#define FT_CHAR_NORM   0x000
#define FT_CHAR_CTRL   0x001
#define FT_CHAR_QUOT   0x002
#define FT_CHAR_LEFT   0x004
#define FT_CHAR_RIGHT  0x008
#define FT_CHAR_YES    0x010
#define FT_CHAR_NO     0x020
#define FT_CHAR_STRONG 0x040
#define FT_CHAR_WEAK   0x080
#define FT_CHAR_NEG    0x100
#define FT_CHAR_TRUNC  0x200
/**
   +a (c "de")
   makes
   CTRL|YES
   NORM
   CTRL
   CTRL|LEFT
   NORM
   CTRL
   CTRL|QUOT|LEFT
   NORM
   NORM
   CTRL|QUOT|RIGHT
   CTRL|RIGHT
*/

class FtCharReader {
public:
	virtual ~FtCharReader(){};
	virtual bool readOne(my_wc_t *wc, int *meta){};
	virtual void reset(){};
};

class FtMemReader : public FtCharReader {
	const char* directBuffer;
	size_t directBufferLength;
	CHARSET_INFO* cs;
	const char* cursor;
public:
	FtMemReader(const char* buffer, size_t bufferLength, CHARSET_INFO *cs);
	~FtMemReader();
	bool readOne(my_wc_t *wc, int *meta);
	void reset();
};

class FtWideReader : public FtCharReader {
	const my_wc_t *buffer;
	size_t bufferLength;
	const my_wc_t *cursor;
public:
	FtWideReader(const my_wc_t *buffer, size_t bufferLength);
	~FtWideReader();
	bool readOne(my_wc_t *wc, int *meta);
	void reset();
};


#if HAVE_ICU
#include <unicode/chariter.h>
#include <unicode/normlzr.h>
#include <unicode/schriter.h>

class WcIterator : public CharacterIterator {
	FtCharReader *reader;
	int control; // control char index
	int controlLength;
	int *metas; // metainfo for control chars
	UnicodeString *cache;
	StringCharacterIterator *cacheIterator;
	size_t formerLength;
	size_t former32Length;
	bool eoc; // End Of Cache (reached final cache)
private:
	void mirror();
public:
	WcIterator(FtCharReader *internal);
	~WcIterator();
	// FowardCharacterIterator
	UBool operator== (const ForwardCharacterIterator &that) const;
	int32_t hashCode() const;
	virtual UClassID getDynamicClassID(void) const { return 0; };
	UChar nextPostInc();
	UChar32 next32PostInc();
	UBool hasNext();
	// CharacterIterator
	CharacterIterator* clone() const;
	UChar first();
	UChar firstPostInc();
	UChar32 first32();
	UChar32 first32PostInc();
	UChar last();
	UChar32 last32();
	UChar setIndex(int32_t position);
	UChar32 setIndex32(int32_t position);
	UChar current() const;
	UChar32 current32() const;
	UChar next();
	UChar32 next32();
	UChar previous();
	UChar32 previous32();
	UBool hasPrevious();
	int32_t move(int32_t delta, EOrigin origin);
	int32_t move32(int32_t delta, EOrigin origin);
	void getText(UnicodeString &result);
	// WcIterator
	void reset();
	bool isEnd();
	int getPreviousControlMeta();
};

class FtNormalizerReader : public FtCharReader {
	WcIterator *wrapper;
	Normalizer *normalizer;
	UNormalizationMode mode;
	bool eos;
public:
	FtNormalizerReader(FtCharReader *internal, UNormalizationMode mode);
	~FtNormalizerReader();
	bool readOne(my_wc_t *wc, int *meta);
	void reset();
	//
	void setOption(int32_t option, UBool value);
};

#endif