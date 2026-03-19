#include "timestamp-tag.h"
#include "ns3/simulator.h"

namespace ns3 {

TimestampTag::TimestampTag () : m_timestamp (Simulator::Now()) {}
TimestampTag::TimestampTag (Time time) : m_timestamp (time) {}

void
TimestampTag::SetTimestamp (Time time)
{
  m_timestamp = time;
}

Time
TimestampTag::GetTimestamp () const
{
  return m_timestamp;
}

TypeId
TimestampTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<TimestampTag> ();
  return tid;
}

TypeId
TimestampTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
TimestampTag::GetSerializedSize (void) const
{
  return 8;
}

void
TimestampTag::Serialize (TagBuffer i) const
{
  int64_t t = m_timestamp.GetNanoSeconds ();
  i.Write ((uint8_t*)&t, 8);
}

void
TimestampTag::Deserialize (TagBuffer i)
{
  int64_t t;
  i.Read ((uint8_t*)&t, 8);
  m_timestamp = NanoSeconds (t);
}

void
TimestampTag::Print (std::ostream &os) const
{
  os << "Timestamp=" << m_timestamp.GetSeconds () << "s";
}

} // namespace ns3
