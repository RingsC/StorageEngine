#include <set>

#include "postgres.h"
#include "interface/pgsubtransaction.h"
#include "access/xact.h"

using std::set;

namespace pgse_interface {

PGSubTransaction::PGSubTransaction(TDTransactionId transaction_id, IsolationLevel isolation_level) : PGTransaction(transaction_id, isolation_level)
{

}


//need object copy for m_child_stransation 
//need modify
PGSubTransaction::PGSubTransaction(const PGSubTransaction &other) :PGTransaction(other)
{
    PGTransaction::PGTransaction(other);
    m_transaction_id = other.getTransactionId();
    m_isolation_level = other.getIsolationLevel();
    m_child_stransation = other.getSubTransaction();

}

PGSubTransaction::~PGSubTransaction()
{
    set<PGTransaction*>::iterator it;
    PGTransaction* tmp;
    for (it = m_child_stransation.begin(); it != m_child_stransation.end(); ++it) {
        tmp = *it;
        if (NULL != tmp) {
            delete [] tmp;
            tmp = NULL;
        }
        m_child_stransation.erase(it);
    }
}

PGSubTransaction& PGSubTransaction::operator = (const PGSubTransaction &other)
{
    PGTransaction::operator =(other);
    if (this != &other) {
        m_transaction_id = other.getTransactionId();
        m_isolation_level = other.getIsolationLevel();
        m_child_stransation = other.getSubTransaction();
    }
    return *this;
}

void PGSubTransaction::abort()
{
    RollbackAndReleaseCurrentSubTransaction();
}

void PGSubTransaction::commit()
{
    EndTransactionBlock();

}

void PGSubTransaction::prepare()
{
    BeginInternalSubTransaction(NULL);
}


} //pgse_interface