#include<stdio.h>
#include<assert.h>
#include<x86intrin.h>


// lets get format nomenclature straight:
// x == don't care bit value

// e.g. for a 32-bit value:

//     7    f    f    8           value
//   x 111 1111 1111 1000  04 08 xxxxxxxx
//   |__________________________________|
//      The entire uint64 is called a *Boxed Value*
//
//
//
//   We can tell that it is indeed a boxed value because:
//
//     7    f    f    8           value
//   x 111 1111 1111 1000  04 08 xxxxxxxx
//     |_____________||__________________|
//      It has all of     ... and it also has 
//      the "NAN" bits    at least one bit
//      set....           set in this region.
///
//
//     7    f    f    8           value
//   x 111 1111 1111 1000  04 08 xxxxxxxx
//                               |______|
//      The value which is boxed is called the *value*.
//      It is always in the last bits of the boxed value.
//
//
//     7    f    f    8      0    
//   x 111 1111 1111 1000  0000 01xx xxxx xxxx (32 more bits for value)
//   |_______________________________________|
//         this is the *type prefix*
//      i.e. everything but the value.
//
//
//
//                                ___  This is the *indicator bit*
//                               |
//     7    f    f    8      0   | 
//   x 111 1111 1111 1000  0000 01xx xxxx xxxx (32 more bits for value)
//                    |______________________|
//                  this is the *type indicator*
//
//  
//     7    f    f    8      0    
//   x 111 1111 1111 1000  0000 01xx xxxx xxxx (32 more bits for value)
//                    |__________|
//     this is the *type base*
//     All values which are the same width in bits will have the
//     same type indicator.
//
//     7    f    f    8      0    
//   x 111 1111 1111 1000  0000 01xx xxxx xxxx (32 more bits for value)
//                                |__________|
//                         this is the *type offset*
//                    it is 1 bit wider than the type base.
// 
//  This is how we distinguish between types of the same width.
//  e.g. we could use different bit patterns to distinguish between
//  a boxed 32-bit integer and a boxed 32-bit floating point number.


typedef long long unsigned int uint64_t;
typedef long long signed int int64_t;
typedef char bool;

const uint64_t MAX_MASK             =  0xffffffffffffffff;
const uint64_t NAN_VALUE            =  0x7ff8000000000000;
const uint64_t MANTISA_MASK         =  0x0007ffffffffffff;
const uint64_t BOXED_MASK           =  0x7fffffffffffffff;
const uint64_t SIGN_MASK            =  0x8000000000000000;
const uint64_t INDICATOR_BIT        =  0x0004000000000000;
const uint64_t CLEAR_SIGN_BIT_MASK  =  0x7fffffffffffffff;


// returns true if this is a boxed type.
// returns false if it is a double
bool is_boxed(const uint64_t val)
{
  return (val & BOXED_MASK) > NAN_VALUE;
}


bool is_double(const uint64_t val)
{
  return !is_boxed(val);
}


uint64_t typePrefix(const int valueWidth, const uint64_t typeOffset)
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
  
  // now that everything is in the right place, just or-them together
  // to construct the type prefix.
  return (NAN_VALUE | indicatorBit | positionedTypeOffset);
}



uint64_t typePrefixMaskForValueWidth(const int valueWidth)
// returns a mask to extract the type prefix from a boxed value
// easy to do if you already know the width of the value
{
  return  (MAX_MASK << valueWidth);
}

  
int indicatorBitPosition(const uint64_t boxedValue)
// returns the position (0-64) of the indicator
// bit of the boxed type.
{
  if(!is_boxed(boxedValue)) return 64;
  
  const uint64_t maskedBoxedValue = boxedValue & MANTISA_MASK;
  const int tmp = _lzcnt_u64(maskedBoxedValue);
  return 63 - tmp;
}


int typeIndicatorWidth(const uint64_t boxedValue)
// returns the number of bits which the type indicator
// takes up.
{
  return 2*(51 - indicatorBitPosition(boxedValue)) + 1;
}  

		   
int typePrefixWidth(const uint64_t boxedValue)
// returns the number of bits which the type indicator
// takes up.
{
  return 51 -  typeIndicatorWidth(boxedValue);
}


typedef struct Packer32 {
  union {
    int as_int;
    float as_float;
  } value;
  int dummy;
} Packer32;


typedef union BoxedValue {
  uint64_t as_uint64;
  double as_double;
  float as_float;
  Packer32 packer32;
} BoxedValue;


//  boxing


const uint64_t INTEGER_STENCIL = 0x7ff8040000000000;
const uint64_t FLOAT_STENCIL   = 0x7ff8040100000000;

BoxedValue boxFloat(const float f){
  BoxedValue returner = (BoxedValue)(FLOAT_STENCIL);
  returner.packer32.value.as_float = f;
  return returner;
}

float unboxFloat(const BoxedValue bv){
  return bv.packer32.value.as_float;
}

		   
BoxedValue boxInt(const int i){
  BoxedValue returner = (BoxedValue)(FLOAT_STENCIL);
  returner.packer32.value.as_int = i;
  return returner;
}

float unboxInt(const BoxedValue bv){
  return bv.packer32.value.as_int;
}

		   

int main(){
  double d = 4.5;
  assert(!is_boxed(d));
  assert(!is_boxed(NAN_VALUE));
  assert(0x7ff8040800000000 == typePrefix(32,8));
  
  {
    const int pos = indicatorBitPosition(0x7ffc000000000000);
    assert(pos == 50);
  }
  
  {
    const int pos = indicatorBitPosition(0x7ff84008ffffffff);
    assert(pos == 46);
  }
  
  for(uint64_t i=0;i<51;i++){
    const uint64_t testBoxedValue
      = ( 0x7ff8000000000000 | ((uint64_t)(0x01) << i));
    const int pos = indicatorBitPosition(testBoxedValue);
    assert(i==pos);
  }
  
  {
    const float in = 4.25;
    const BoxedValue bv = boxFloat(in);
    const float out = unboxFloat(bv);
    assert(in == out);
  }
  
  
  {
    const int in = 42;
    const BoxedValue bv = boxInt(in);
    const int out = unboxInt(bv);
    assert(in == out);
  }
  
  {
    const int pos = indicatorBitPosition(0x7ff80408ffffffff);
    const int width = typeIndicatorWidth(0x7ff80408ffffffff);
    const int owidth = typeIndicatorWidth(32);
    const int tpwidth = typePrefixWidth(0x7ff80408ffffffff);
  }
  
  
  for(int i=48; i>=0; i=i-2){
    printf("value width = %d   type prefix = %016llX\n",
	   i, typePrefix(i,(uint64_t)(0x00)));
  }
  
}
