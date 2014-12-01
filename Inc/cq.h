#ifndef __CQ_H__
#define __CQ_H__

#define CQ_DEFINE(cq_type,cq_name,cq_count)   cq_type cq_name[cq_count]; volatile unsigned short cq_name##Front=0; volatile unsigned short cq_name##Rear=0; const unsigned short cq_name##Size=cq_count;
#define CQ_EXTERN(cq_type,cq_name,cq_count)   extern cq_type cq_name[cq_count]; extern volatile unsigned short cq_name##Front; extern volatile unsigned short cq_name##Rear; extern const unsigned short cq_name##Size;

#define CQ_INC_PTR(cq, ptr)         (((ptr)+1)%cq##Size)
#define CQ_DEC_PTR(cq, ptr)         ((ptr)==0 ? cq##Size-1 : (ptr)-1)
#define CQ_NEXT_PTR(cq, ptr)        CQ_INC_PTR(cq, ptr)
#define CQ_PREV_PTR(cq, ptr)        CQ_DEC_PTR(cq, ptr)

#define CQ_PTR_ISVALID(cq,ptr)      ((cq##Front<cq##Rear && (ptr>=cq##Front && ptr<cq##Rear)) || (cq##Front>cq##Rear && (ptr>=cq##Front || ptr<cq##Rear)))

#define CQ_FRONT(cq)                (cq##Front)
#define CQ_REAR(cq)                 (cq##Rear)
#define CQ_MAX(cq)                  (cq##Size)

#define CQ_OUTPUT_PTR(cq)           (cq##Front)
#define CQ_INPUT_PTR(cq)            (cq##Rear)

#define CQ_CONTINUOUS_LEN(cq)       (cq##Front <= cq##Rear ? cq##Rear - cq##Front : cq##Size - cq##Front)

#define CQ_SIZE(cq)                 (cq##Size-1) // (sizeof(cq_name)/sizeof(cq_name[0]))
#define CQ_USEDSIZE(cq)             ((cq##Rear>=cq##Front)?(cq##Rear-cq##Front):(cq##Size-cq##Front+cq##Rear))
#define CQ_FREESIZE(cq)             ((cq##Rear>=cq##Front)?(cq##Size-cq##Rear+cq##Front):(cq##Front-cq##Rear))

#define CQ_COUNT(cq,front,rear)     ((rear>=front)?(rear-front):(cq##Size-front+rear))

#define CQ_ISEMPTY(cq)              (cq##Front == cq##Rear)
#define CQ_ISNOTEMPTY(cq)           (cq##Front != cq##Rear)
#define CQ_ISFULL(cq)               (cq##Front == ((cq##Rear<cq##Size-1)?cq##Rear+1:0))
#define CQ_ISNOTFULL(cq)            (cq##Front != ((cq##Rear<cq##Size-1)?cq##Rear+1:0))

#define CQ_PUT(cq,data)             do { (cq)[cq##Rear]=data; if (cq##Rear<cq##Size-1) cq##Rear++; else cq##Rear=0; } while(0)
#define CQ_GET(cq,data)             do { data=(cq)[cq##Front]; if (cq##Front<cq##Size-1) cq##Front++; else cq##Front=0; } while(0)

#define CQ_PEEK(cq,data)            (data=cq[cq##Front])
#define CQ_PEEK_IDX(cq,data,idx)    (cq##Front+(idx)<cq##Size ? (data=cq[cq##Front+(idx)]) : (data=cq[cq##Front+(idx)-cq##Size]))
#define CQ_PEEK_DIRECT(cq)          (cq[cq##Front])
#define CQ_PEEK_IDX_DIRECT(cq,idx)  (cq##Front+(idx)<cq##Size ? (cq[cq##Front+(idx)]) : (cq[cq##Front+(idx)-cq##Size]))

#define CQ_PUTBUF(cq,buf,size)      for ( int cq_put_copy_idx=0 ; cq_put_copy_idx<size && cq##Front!=((cq##Rear<cq##Size-1)?cq##Rear+1:0) ; ) { (cq)[cq##Rear]=(buf)[cq_put_copy_idx++]; if (cq##Rear<cq##Size-1) cq##Rear++; else cq##Rear=0; }
#define CQ_GETBUF(cq,buf,size)      for ( int cq_get_copy_idx=0 ; cq_get_copy_idx<size && cq##Front!=cq##Rear ; ) { (buf)[cq_get_copy_idx++]=(cq)[cq##Front]; if (cq##Front<cq##Size-1) cq##Front++; else cq##Front=0; }

#define CQ_INIT(cq)                 do { cq##Front = cq##Rear = 0; } while(0)
#define CQ_FLUSH(cq)                do { cq##Front = cq##Rear; } while(0)
#define CQ_DROP(cq,size)            if ( cq##Rear >= cq##Front ) { \
                                        cq##Front += (size); \
                                        if ( cq##Front > cq##Rear ) cq##Front = cq##Rear; \
                                    } else { \
                                        cq##Front += (size); \
                                        if ( cq##Front >= cq##Size ) { \
                                            cq##Front -= cq##Size; \
                                            if ( cq##Front > cq##Rear ) cq##Front = cq##Rear; \
                                        } \
                                    }

#endif
