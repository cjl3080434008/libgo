#pragma once
#include <unistd.h>
#include <vector>
#include <list>
#include <set>
#include "task.h"

namespace co
{

    class IoWait
    {
    public:
        IoWait();

        // ��Э���е��õ�switch, �ݴ�״̬��yield
        void CoSwitch(std::vector<FdStruct> && fdsts, int timeout_ms);

        // �ڵ������е��õ�switch, ����ɹ������ȴ����У����ʧ�������¼ӻ�runnable����
        void SchedulerSwitch(Task* tk);

        int WaitLoop(bool enable_block);

        void DelayEventWaitTime();
        void ResetEventWaitTime();

    private:
        void Cancel(Task *tk, uint32_t id);

        int epollwait_ms_;
    };


} //namespace co