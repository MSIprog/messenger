#include "signaling_facade.h"

SignalingFacade::SignalingFacade(quint16 a_port)
{
    m_signaling = std::make_shared<Signaling>();
    if (!m_signaling->start())
    {
        m_signaling = nullptr;
        return;
    }

    if (!m_detectionServer.start(m_signaling->getPort(), a_port))
    {
        m_signaling = nullptr;
        return;
    }
    QObject::connect(&m_detectionServer, &DetectionServer::peerFound, m_signaling.get(), &Signaling::addPeer);

    m_seekerClient.start(m_signaling->getPort(), a_port);
}
