#ifndef PICPOLL_H
#define PICPOLL_H
#include "common_utils.h"

class PicPoll {
public:
    PicPoll(int numQueues, int maxElements) : numQueues(numQueues), maxElements(maxElements) {
        for (int i = 0; i < numQueues; ++i) {
            QQueue<QByteArray> queue;
            m_queues.append(queue);
        }
    }
    //向指定队列存入指定元素
    void addToQueue(int queueIndex, const QByteArray data) {
        if (queueIndex < 0 || queueIndex >= numQueues) {
            return;
        }

        QQueue<QByteArray> &queue = m_queues[queueIndex];
        if (queue.size() >= maxElements) {
            queue.dequeue();
        }
        queue.enqueue(data);
    }
    //照片匹配规则
    void matchingPicFromPool(STR_MSG* source,int pictype)
    {
        int pos = source->firstcarpicpos;//以第一张照片的车牌和时间为参考物,与其他比较
        source->carpics[source->firstcarpicpos];//最先到达的照片
        source->carnos[source->firstcarpicpos];//最先到达的车牌
        //匹配当前车辆pictype
        if(strcmp(source->carnos[pos], source->carnos[pictype]) != 0)//不相等的车牌时
        {
            if((strstr(source->carnos[pictype],"挂") != NULL))//含挂的车牌
            {
            }
            else
            {
                //遍历队列是否有该车的照片
                QQueue<QByteArray>::const_iterator iter;
                for (iter = m_queues[pictype].constBegin(); iter != m_queues[pictype].constEnd(); ++iter) {
                    if( compareStrings(iter->data()+39,source->carnos[pos])){
                        strcpy(source->carpics[pictype], iter->data());

                        break;//匹配到了,结束
                    }
                }
            }
        }
    }

    QByteArray findInQueue(int queueIndex, const QByteArray &data) {
        //前后不一致
        /*if(strcmp(it.second->strcarno2, it.second->strcarno) != 0)
        {
            if((strstr(it.second->strcarno2,"挂") != NULL))
            {
                if(strstr(it.second->strcarno, "无") != NULL)
                    strcpy(it.second->strcarno, it.second->strcarno2);
            }
            else
            {
                OutputLog("%s ***pass*** hou %s", it.second->strcarno, it.second->strcarno2);
                flag_aft=0;
                //遍历队列是否有该车的照片
                QQueue<QByteArray>::const_iterator iter;
                for (iter = afrQueue.constBegin(); iter != afrQueue.constEnd(); ++iter) {
//                                        OutputLog("ks----- str %s:%s ", it.second->strcarno,iter->data()+39);
//                                        if(strstr(iter->data(), it.second->strcarno+4) != NULL){
                    if( compareStrings(iter->data()+39,it.second->strcarno)){
                        OutputLog("ks----- houuu TTTTTT %s:%s",iter->data(),it.second->strafrpic);
                        strcpy(it.second->strafrpic, iter->data());
                        flag_aft=1;
                        break;
                    }
                    else{
                        flag_aft=0;
                    }
                }
            }
        }*/
        ///////////////////////////////////////////
        if (queueIndex < 0 || queueIndex >= numQueues) {
            qWarning() << "Invalid queue index";
            return QByteArray();
        }

        const QQueue<QByteArray> &queue = m_queues[queueIndex];
        for (const QByteArray &element : queue) {
            if (element == data) {
                return element;
            }
        }

        return QByteArray();
    }

    int numQueues;//一共有几个照片
private:
    int maxElements;//每个队列最多缓存多少照片
    QVector<QQueue<QByteArray>> m_queues;//所有队列
    bool compareStrings(const char* str1, const char* str2) {
        bool issame= false;
        // 查找数字部分
        const char* digitStr1 = str1;
        const char* digitStr2 = str2;
        while (*digitStr1 != '\0' && *digitStr2 != '\0') {
            if(*digitStr1>47 && *digitStr1<57)
            {
                if(*digitStr2<47 || *digitStr2>57){
                    issame=false;
                    break;
                }
                if(atoi(digitStr1)==atoi(digitStr2)){
                    issame=true;
    //                OutputLog("--- compare true : %s  %s", str1,str2);
                }
                else{
                    issame=false;
                    break;
                }
            }
            digitStr1++;
            digitStr2++;
        }
        return issame;
    }

};

#endif // PICPOLL_H
