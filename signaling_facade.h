#pragma once

#include "signaling.h"
#include "detection_server.h"
#include "seeker_client.h"

class SignalingFacade
{
public:
    SignalingFacade(quint16 a_port = 1234);

    std::shared_ptr<Signaling> getSignaling()
    {
        return m_signaling;
    }

private:
    DetectionServer m_detectionServer;
    SeekerClient m_seekerClient;
    std::shared_ptr<Signaling> m_signaling;
};
