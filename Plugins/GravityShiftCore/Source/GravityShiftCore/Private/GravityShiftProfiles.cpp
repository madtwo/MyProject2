#include "GravityShiftProfiles.h"

UGSGravityRuleSet::UGSGravityRuleSet()
{
    AllowedAxes =
    {
        EGSGravityAxis::ZNegative,
        EGSGravityAxis::XPositive,
        EGSGravityAxis::ZPositive,
        EGSGravityAxis::XNegative,
        EGSGravityAxis::YPositive,
        EGSGravityAxis::YNegative
    };
}
