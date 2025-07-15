#include<stdio.h>
#include<assert.h>
#include<string.h>
#include<x86intrin.h>
#include<iostream>


struct BoxedValue{
  constexpr static const uint64_t MAX_MASK             =  0xffffffffffffffff;
  constexpr static const uint64_t NAN_VALUE            =  0x7ff8000000000000;
  constexpr static const uint64_t MANTISA_MASK         =  0x0007ffffffffffff;
  constexpr static const uint64_t BOXED_MASK           =  0x7fffffffffffffff;
  constexpr static const uint64_t SIGN_MASK            =  0x8000000000000000;
  constexpr static const uint64_t INDICATOR_BIT        =  0x0004000000000000;
  constexpr static const uint64_t CLEAR_SIGN_BIT_MASK  =  0x7fffffffffffffff;
  
  struct Packer32 {
    union {
      int as_int;
      float as_float;
    } value;
    int dummy;
  };

  union {
    uint64_t as_uint64;
    double as_double;
    
    // 32-bit payloads:
    int as_int;
    float as_float;
    
    // packer utility
    Packer32 packer32;
  } bv;
  
  
  // returns true if this is a boxed type.
  // returns false if it is a double
  constexpr static bool is_boxed(const uint64_t val)
  {
    return (val & BOXED_MASK) > NAN_VALUE;
  }


  constexpr static bool is_double(const uint64_t val)
  {
    return !is_boxed(val);
  }


  static uint64_t typePrefix(const int valueWidth, const uint64_t typeOffset)
  // What all the leading bits of a boxed value should be,
  // except for the final bits which hold the value.
  {
    // figure the size and format of the type indicator
    const int indicatorLength = 51-valueWidth;
    const int numbZeros = indicatorLength/2-1;
    
    // position the indicator bit
    const uint64_t indicatorBit
      = (INDICATOR_BIT >> numbZeros);
    
    // wrangle the type offset into place.
    const uint64_t maskedTypeOffset
      = (~(MAX_MASK << (numbZeros+2)) & typeOffset);
    
    const uint64_t positionedTypeOffset
      = maskedTypeOffset << valueWidth;
    
    // now that everything is in the right place, just or-them
    // together to construct the type prefix.
    return (NAN_VALUE | indicatorBit | positionedTypeOffset);
  }

  
  static uint64_t typePrefixMaskForValueWidth(const int valueWidth)
  // returns a mask to extract the type prefix from a boxed value
  // easy to do if you already know the width of the value
  {
    return (MAX_MASK << valueWidth);
  }

  
  static int indicatorBitPosition(const uint64_t boxedValue)
  // returns the position (0-64) of the indicator
  // bit of the boxed type.
  {
    if(is_double(boxedValue)) return 64;
    
    const uint64_t maskedBoxedValue = boxedValue & MANTISA_MASK;
    const int tmp = _lzcnt_u64(maskedBoxedValue);
    return 63 - tmp;
  }

  
  static int typeIndicatorWidth(const uint64_t boxedValue)
  // returns the number of bits which the type indicator
  // takes up.
  {
    return 2*(51 - indicatorBitPosition(boxedValue)) + 1;
  }  
  
		   
  static int typePrefixWidth(const uint64_t boxedValue)
  // returns the number of bits which the type indicator
  // takes up.
  {
    return 51 -  typeIndicatorWidth(boxedValue);
  }
  


  //--------
  // boxing
  //--------
  
  // integer suport
  constexpr static const uint64_t INT_STENCIL =  0x7ff8040000000000;
  static BoxedValue boxInt(const int i){
    BoxedValue returner;
    returner.bv.as_uint64 = INT_STENCIL;
    returner.bv.packer32.value.as_int = i;
    return returner;
  }
  
  static int unboxInt(const BoxedValue bv){
    return bv.bv.packer32.value.as_int;
  }
  
  
  // float support
  constexpr static const uint64_t FLOAT_STENCIL = 0x7ff8040100000000;
  static BoxedValue boxFloat(const float f){
    BoxedValue returner;
    returner.bv.as_uint64 = FLOAT_STENCIL;
    returner.bv.packer32.value.as_float = f;
    return returner;
  }
  
  static float unboxFloat(const BoxedValue bv)
  {
    return bv.bv.packer32.value.as_float;
  }
  
  // char* support
  constexpr static const uint64_t CHAR_PTR_STENCIL = 0x7FFC000000000000;
  constexpr static const uint64_t CHAR_PTR_MASK    =  ~(MAX_MASK << 48);
  static BoxedValue boxCharPtr(char *ptr)
  {
    BoxedValue returner;
    returner.bv.as_uint64 = (CHAR_PTR_STENCIL | (uint64_t)ptr);
    return returner;
  }
  
  static char* unboxCharPtr(const BoxedValue bv)
  {
    return (char*)(bv.bv.as_uint64 & CHAR_PTR_MASK);
  }


  // const char* support
  constexpr static const uint64_t CONST_CHAR_PTR_STENCIL = 0x7FFD000000000000;
  constexpr static const uint64_t CONST_CHAR_PTR_MASK    =  ~(MAX_MASK << 48);

  static BoxedValue boxConstCharPtr(const char *ptr)
  {
    BoxedValue returner;
    returner.bv.as_uint64 =(CONST_CHAR_PTR_STENCIL | (uint64_t)ptr);
    return returner;
  }
  
  static const char* unboxConstCharPtr(const BoxedValue bv)
  {
    return (const char*)(bv.bv.as_uint64 & CONST_CHAR_PTR_MASK);
  }
  
  // boolean support
  constexpr static const uint64_t BOOL_STENCIL = 0x7FF8000008000000;
  constexpr static const uint64_t BOOL_MASK    = 0x0000000000000001;
  static BoxedValue boxBool(const bool bit)
  {
    BoxedValue returner;
    returner.bv.as_uint64 = (BOOL_STENCIL | (uint64_t)bit);
    return returner;
  }
  
  static bool unboxBool(const BoxedValue bv)
  {
    return (bool)(bv.bv.as_uint64 & BOOL_MASK);
  }
  
};


class BoxedValueStack : public std::vector<BoxedValue>
// expression stack for the calculator
{
};


class Calculator {
  BoxedValueStack stack;
public:
};

  


int main(){
  {
    // ensure we haven't bloated the BoxedValue
    assert(sizeof(uint64_t) == sizeof(BoxedValue));
    assert(8 == sizeof(BoxedValue));
  }

  {
    double d = 4.5;
    assert(!BoxedValue::is_boxed(d));
    assert(!BoxedValue::is_boxed(BoxedValue::NAN_VALUE));
    assert(0x7ff8040800000000 == BoxedValue::typePrefix(32,8));
  }

  {
    // test out the indicatorBitPosition function
    {
      const int pos = BoxedValue::indicatorBitPosition(0x7ffc000000000000);
      assert(pos == 50);
    }
    
    {
      const int pos = BoxedValue::indicatorBitPosition(0x7ff84008ffffffff);
      assert(pos == 46);
    }
  
    for(uint64_t i=0;i<51;i++){
      const uint64_t testBoxedValue
	= ( 0x7ff8000000000000 | ((uint64_t)(0x01) << i));
      const int pos = BoxedValue::indicatorBitPosition(testBoxedValue);
      assert(i==pos);
    }
  }
  
  {
    // test float boxing
    const float in = 4.25;
    const BoxedValue bv = BoxedValue::boxFloat(in);
    const float out = BoxedValue::unboxFloat(bv);
    assert(in == out);
  }
  
  {
    const int in = 42;
    const BoxedValue bv = BoxedValue::boxInt(in);
    const int out = BoxedValue::unboxInt(bv);
    assert(in == out);
  }
  
  {
    char bluf[20];
    char* in = bluf;
    strcpy(in, "This is a test");
    const BoxedValue bv = BoxedValue::boxCharPtr(in);
    char* out = BoxedValue::unboxCharPtr(bv);
    assert(in == out);
    assert(!strcmp(in, out));
  }
  
  {
    const char* in = "This is a test";
    const BoxedValue bv = BoxedValue::boxConstCharPtr(in);
    const char* out = BoxedValue::unboxConstCharPtr(bv);
    assert(in == out);
    assert(!strcmp(in, out));
  }
  
  {
    const int pos = BoxedValue::indicatorBitPosition(0x7ff80408ffffffff);
    const int width = BoxedValue::typeIndicatorWidth(0x7ff80408ffffffff);
    const int owidth = BoxedValue::typeIndicatorWidth(32);
    const int tpwidth = BoxedValue::typePrefixWidth(0x7ff80408ffffffff);
  }
  
  {
    BoxedValue bv;
    const bool in0 = 0;
    bv = BoxedValue::boxBool(in0);
    const bool out0 = BoxedValue::unboxBool(bv);
    assert(in0 == out0);
    
    const bool in1 = 1;
    bv = BoxedValue::boxBool(in1);
    const bool out1 = BoxedValue::unboxBool(bv);
    assert(in1 == out1);
  }
  
  for(int i=48; i>=0; i=i-2){
    printf("value width = %d   type prefix = %016llX\n",
	   i, BoxedValue::typePrefix(i,(uint64_t)(0x00)));
  }
  
}
