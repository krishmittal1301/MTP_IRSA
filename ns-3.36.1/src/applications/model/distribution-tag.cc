#include "distribution-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistributionTag");

DistributionTag::DistributionTag ()
  : m_distribution (CONSTANT)
{
}

DistributionTag::DistributionTag (DistType dist)
  : m_distribution (static_cast<uint8_t>(dist))
{
}

void DistributionTag::SetDistribution (DistType dist)
{
  m_distribution = static_cast<uint8_t>(dist);
}

DistributionTag::DistType DistributionTag::GetDistribution () const
{
  return static_cast<DistType>(m_distribution);
}

TypeId
DistributionTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DistributionTag")
    .SetParent<Tag> ()
    .AddConstructor<DistributionTag> ();
  return tid;
}

TypeId
DistributionTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
DistributionTag::GetSerializedSize (void) const
{
  return 1; // one byte
}

void
DistributionTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_distribution);
}

void
DistributionTag::Deserialize (TagBuffer i)
{
  m_distribution = i.ReadU8 ();
}

void
DistributionTag::Print (std::ostream &os) const
{
  os << "DistType=" << static_cast<uint32_t>(m_distribution);
}

} // namespace ns3
