#include "../Crypto/CryptoFactory.hpp"
#include "Node.hpp"
#include "SessionFactory.hpp"

namespace Dissent {
namespace Applications {
  Node::Node(const Id &local_id, const QList<Address> &local,
      const QList<Address> &remote, int group_size,
      const QString &session_type) :
    bg(local_id, local, remote),
    sm(bg.GetRpcHandler()),
    GroupSize(group_size),
    SessionType(session_type),
    _bootstrapped(false)
  {
    QObject::connect(&bg, SIGNAL(NewConnection(Connection *, bool)),
        this, SLOT(HandleConnection(Connection *, bool)));
  }

  Node::~Node()
  {
    QObject::disconnect(this, SIGNAL(Ready()), 0 ,0);
  }

  void Node::HandleConnection(Connection *, bool local)
  {
    if(!local) {
      return;
    }

    QList<Connection *> cons = bg.GetConnectionTable().GetConnections();
    if(cons.count() != GroupSize) {
      return;
    }

    QObject::disconnect(&bg, SIGNAL(NewConnection(Connection *, bool)),
        this, SLOT(HandleConnection(Connection *, bool)));
    SessionFactory::GetInstance().Create(this, SessionType);
    _bootstrapped = true;
    emit Ready();
    QObject::disconnect(this, SIGNAL(Ready()), 0 ,0);
  }

  void Node::RoundFinished(QSharedPointer<Round>)
  {
  }

  Group Node::GenerateGroup()
  {
    QVector<GroupContainer> group_roster;
    Library *lib = CryptoFactory::GetInstance().GetLibrary();

    foreach(Connection *con, bg.GetConnectionTable().GetConnections()) {
      Id id = con->GetRemoteId();
      QSharedPointer<AsymmetricKey> key(lib->GeneratePublicKey(id.GetByteArray()));
      QScopedPointer<DiffieHellman> dh(lib->GenerateDiffieHellman(id.GetByteArray()));
      group_roster.append(GroupContainer(id, key, dh->GetPublicComponent()));
    }

    qSort(group_roster);

    return Group(group_roster);
  }
}
}
