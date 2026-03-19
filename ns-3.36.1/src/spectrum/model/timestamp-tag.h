#ifndef TIMESTAMP_TAG_H
#define TIMESTAMP_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class TimestampTag : public Tag
{
public:
  TimestampTag ();
  TimestampTag (Time time);

  void SetTimestamp (Time time);
  Time GetTimestamp () const;

  // Required NS-3 functions
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  Time m_timestamp;
};

} // namespace ns3

#endif // TIMESTAMP_TAG_H
