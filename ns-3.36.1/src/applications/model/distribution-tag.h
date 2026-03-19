#ifndef DISTRIBUTION_TAG_H
#define DISTRIBUTION_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class DistributionTag : public Tag
{
public:
  enum DistType {
      EXPONENTIAL = 0,
      UNIFORM = 1,
      NORMAL = 2,
      CONSTANT = 3,
  };

  DistributionTag ();
  DistributionTag (DistType dist);

  void SetDistribution (DistType dist);
  DistType GetDistribution () const;

  // Required NS-3 functions
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  uint8_t m_distribution; // store enum as byte
};

} // namespace ns3

#endif // DISTRIBUTION_TAG_H
