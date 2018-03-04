#ifndef PG_SUB_TRANSACTION_H
#define PG_SUB_TRANSACTION_H
#include "interface/pgtransaction.h"

namespace pgse_interface {
class PGSubTransaction : public PGTransaction
{
public:
    ///constructor
        PGSubTransaction(TDTransactionId transaction_id = InvalidTransactionId, IsolationLevel isolation_level = NO_ISOLATION_LEVEL_SPECIFIED);
        PGSubTransaction(const PGSubTransaction &other);
    /// overload operator =
        PGSubTransaction& operator = (const PGSubTransaction &other);

    ///destructor
        virtual ~PGSubTransaction();

    /// Abort the transaction.
        virtual void abort();

    /// Commit the transaction.
        virtual void commit();

    /// Prepare the transaction.
        virtual void prepare();
    //test
        virtual void setlevel(Transaction::IsolationLevel level){
            m_isolation_level = level;
        }

};

} //pgse_interface

#endif //