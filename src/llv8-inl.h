#ifndef SRC_LLV8_INL_H_
#define SRC_LLV8_INL_H_

#include "llv8.h"

namespace llnode {
namespace v8 {

template <class T>
T LLV8::LoadValue(int64_t addr, Error& err) {
  int64_t ptr;
  ptr = LoadPtr(addr, err);
  if (err.Fail()) return T();

  T res = T(this, ptr);
  if (!res.Check()) {
    err = Error::Failure("Invalid value");
    return T();
  }

  return res;
}


bool Smi::Check() const {
  return (raw() & v8()->smi_.kTagMask) == v8()->smi_.kTag;
}


int64_t Smi::GetValue() const {
  return raw() >> (v8()->smi_.kShiftSize + v8()->smi_.kTagMask);
}


bool HeapObject::Check() const {
  return (raw() & v8()->heap_obj_.kTagMask) == v8()->heap_obj_.kTag;
}


int64_t HeapObject::LeaField(int64_t off) const {
  return raw() - v8()->heap_obj_.kTag + off;
}


int64_t HeapObject::LoadField(int64_t off, Error& err) {
  return v8()->LoadPtr(LeaField(off), err);
}


template <class T>
T HeapObject::LoadFieldValue(int64_t off, Error& err) {
  T res = v8()->LoadValue<T>(LeaField(off), err);
  if (err.Fail()) return T();
  if (!res.Check()) {
    err = Error::Failure("Invalid value");
    return T();
  }

  return res;
}


int64_t HeapObject::GetType(Error& err) {
  HeapObject obj = GetMap(err);
  if (err.Fail()) return -1;

  Map map(obj);
  return map.GetType(err);
}


int64_t Map::GetType(Error& err) {
  int64_t type = LoadField(v8()->map_.kInstanceAttrsOffset, err);
  if (err.Fail()) return -1;
  return type & 0xff;
}


int64_t JSFrame::LeaParamSlot(int slot, int count) const {
  return raw() + v8()->frame_.kArgsOffset +
      (count - slot - 1) * v8()->kPointerSize;
}


Value JSFrame::GetReceiver(int count, Error &err) {
  return GetParam(-1, count, err);
}


Value JSFrame::GetParam(int slot, int count, Error& err) {
  int64_t addr = LeaParamSlot(slot, count);
  return v8()->LoadValue<Value>(addr, err);
}


std::string JSFunction::Name(Error& err) {
  SharedFunctionInfo info = Info(err);
  if (err.Fail()) return std::string();

  return Name(info, err);
}


bool Map::IsDictionary(Error& err) {
  int64_t field = BitField3(err);
  if (err.Fail()) return false;

  return (field & (1 << v8()->map_.kDictionaryMapShift)) != 0;
}


int64_t Map::NumberOfOwnDescriptors(Error& err) {
  int64_t field = BitField3(err);
  if (err.Fail()) return false;

  // TODO(indutny): this should really be controlled by postmortem data
  // Skip EnumLength
  field >>= (v8()->map_.kDictionaryMapShift - kDescriptorIndexBitCount);
  field &= (1 << kDescriptorIndexBitCount) - 1;
  return field;
}


#define ACCESSOR(CLASS, METHOD, OFF, TYPE)                                    \
    TYPE CLASS::METHOD(Error& err) {                                          \
      return LoadFieldValue<TYPE>(v8()->OFF, err);                            \
    }


ACCESSOR(HeapObject, GetMap, heap_obj_.kMapOffset, HeapObject)

ACCESSOR(Map, MaybeConstructor, map_.kMaybeConstructorOffset, HeapObject)
ACCESSOR(Map, InstanceDescriptors, map_.kInstanceDescriptorsOffset, HeapObject)

int64_t Map::BitField3(Error& err) {
  return LoadField(v8()->map_.kBitField3Offset, err) & 0xffffffff;
}

int64_t Map::InObjectProperties(Error& err) {
  return LoadField(v8()->map_.kInObjectPropertiesOffset, err) & 0xff;
}

int64_t Map::InstanceSize(Error& err) {
  return (LoadField(v8()->map_.kInstanceSizeOffset, err) & 0xff) *
      v8()->kPointerSize;
}

ACCESSOR(JSObject, Properties, js_object_.kPropertiesOffset, HeapObject)
ACCESSOR(JSObject, Elements, js_object_.kElementsOffset, HeapObject)

ACCESSOR(JSArray, Length, js_array_.kLengthOffset, Smi)

int64_t String::Representation(Error& err) {
  int64_t type = GetType(err);
  if (err.Fail()) return -1;
  return type & v8()->string_.kRepresentationMask;
}


int64_t String::Encoding(Error& err) {
  int64_t type = GetType(err);
  if (err.Fail()) return -1;
  return type & v8()->string_.kEncodingMask;
}

ACCESSOR(String, Length, string_.kLengthOffset, Smi)

ACCESSOR(Script, Name, script_.kNameOffset, String)
ACCESSOR(Script, LineOffset, script_.kLineOffsetOffset, Smi)
ACCESSOR(Script, Source, script_.kSourceOffset, HeapObject)
ACCESSOR(Script, LineEnds, script_.kLineEndsOffset, HeapObject)

ACCESSOR(SharedFunctionInfo, Name, shared_info_.kNameOffset, String)
ACCESSOR(SharedFunctionInfo, InferredName, shared_info_.kInferredNameOffset,
         String)
ACCESSOR(SharedFunctionInfo, GetScript, shared_info_.kScriptOffset, Script)

ACCESSOR(Oddball, Kind, oddball_.kKindOffset, Smi)

int64_t JSArrayBuffer::BackingStore(Error& err) {
  return LoadField(v8()->js_array_buffer_.kBackingStoreOffset, err);
}

ACCESSOR(JSArrayBuffer, ByteLength, js_array_buffer_.kByteLengthOffset, Smi)

ACCESSOR(JSArrayBufferView, Buffer, js_array_buffer_view_.kBufferOffset,
         JSArrayBuffer)
ACCESSOR(JSArrayBufferView, ByteOffset, js_array_buffer_view_.kByteOffsetOffset,
         Smi)
ACCESSOR(JSArrayBufferView, ByteLength, js_array_buffer_view_.kByteLengthOffset,
         Smi)

// TODO(indutny): this field is a Smi on 32bit
int64_t SharedFunctionInfo::ParameterCount(Error& err) {
  int64_t field = LoadField(v8()->shared_info_.kParameterCountOffset, err);
  if (err.Fail()) return -1;

  field &= 0xffffffff;
  return field;
}

// TODO(indutny): this field is a Smi on 32bit
int64_t SharedFunctionInfo::StartPosition(Error& err) {
  int64_t field = LoadField(v8()->shared_info_.kStartPositionOffset, err);
  if (err.Fail()) return -1;

  field &= 0xffffffff;
  field >>= v8()->shared_info_.kStartPositionShift;
  return field;
}

ACCESSOR(JSFunction, Info, js_function_.kSharedInfoOffset, SharedFunctionInfo);

ACCESSOR(ConsString, First, cons_string_.kFirstOffset, String);
ACCESSOR(ConsString, Second, cons_string_.kSecondOffset, String);

ACCESSOR(SlicedString, Parent, sliced_string_.kParentOffset, String);
ACCESSOR(SlicedString, Offset, sliced_string_.kOffsetOffset, Smi);

ACCESSOR(FixedArrayBase, Length, fixed_array_base_.kLengthOffset, Smi);

std::string OneByteString::ToString(Error& err) {
  int64_t chars = LeaField(v8()->one_byte_string_.kCharsOffset);
  Smi len = Length(err);
  if (err.Fail()) return std::string();
  return v8()->LoadString(chars, len.GetValue(), err);
}

std::string TwoByteString::ToString(Error& err) {
  int64_t chars = LeaField(v8()->two_byte_string_.kCharsOffset);
  Smi len = Length(err);
  if (err.Fail()) return std::string();
  return v8()->LoadTwoByteString(chars, len.GetValue(), err);
}

std::string ConsString::ToString(Error& err) {
  String first = First(err);
  if (err.Fail()) return std::string();

  String second = Second(err);
  if (err.Fail()) return std::string();

  std::string tmp = first.ToString(err);
  if (err.Fail()) return std::string();
  tmp += second.ToString(err);
  if (err.Fail()) return std::string();

  return tmp;
}

std::string SlicedString::ToString(Error& err) {
  String parent = Parent(err);
  if (err.Fail()) return std::string();

  Smi offset = Offset(err);
  if (err.Fail()) return std::string();

  Smi length = Length(err);
  if (err.Fail()) return std::string();

  std::string tmp = parent.ToString(err);
  if (err.Fail()) return std::string();

  return tmp.substr(offset.GetValue(), length.GetValue());
}

int64_t FixedArray::LeaData() const {
  return LeaField(v8()->fixed_array_.kDataOffset);
}

template <class T>
T FixedArray::Get(int index, Error& err) {
  int64_t off = v8()->fixed_array_.kDataOffset + index * v8()->kPointerSize;
  return LoadFieldValue<T>(off, err);
}

Smi DescriptorArray::GetDetails(int index, Error& err) {
  return Get<Smi>(v8()->descriptor_array_.kFirstIndex +
                      index * v8()->descriptor_array_.kSize +
                      v8()->descriptor_array_.kDetailsOffset,
                  err);
}

Value DescriptorArray::GetKey(int index, Error& err) {
  return Get<Value>(v8()->descriptor_array_.kFirstIndex +
                        index * v8()->descriptor_array_.kSize +
                        v8()->descriptor_array_.kKeyOffset,
                    err);
}

bool DescriptorArray::IsFieldDetails(Smi details) {
  return (details.GetValue() & v8()->descriptor_array_.kPropertyTypeMask) ==
      v8()->descriptor_array_.kFieldType;
}

int64_t DescriptorArray::FieldIndex(Smi details) {
  return (details.GetValue() & v8()->descriptor_array_.kPropertyIndexMask) >>
      v8()->descriptor_array_.kPropertyIndexShift;
}

Value NameDictionary::GetKey(int index, Error& err) {
  int64_t off = v8()->name_dictionary_.kPrefixSize +
      index * v8()->name_dictionary_.kEntrySize +
      v8()->name_dictionary_.kKeyOffset;
  return FixedArray::Get<Value>(off, err);
}

Value NameDictionary::GetValue(int index, Error& err) {
  int64_t off = v8()->name_dictionary_.kPrefixSize +
      index * v8()->name_dictionary_.kEntrySize +
      v8()->name_dictionary_.kValueOffset;
  return FixedArray::Get<Value>(off, err);
}

int64_t NameDictionary::Length(Error& err) {
  Smi length = FixedArray::Length(err);
  if (err.Fail()) return -1;

  int64_t res = length.GetValue() - v8()->name_dictionary_.kPrefixSize;
  res /= v8()->name_dictionary_.kEntrySize;
  return res;
}

bool Oddball::IsHoleOrUndefined(Error& err) {
  Smi kind = Kind(err);
  if (err.Fail()) return false;

  return kind.GetValue() == v8()->oddball_.kTheHole ||
         kind.GetValue() == v8()->oddball_.kUndefined;
}

#undef ACCESSOR

}  // namespace v8
}  // namespace llnode

#endif  // SRC_LLV8_INL_H_