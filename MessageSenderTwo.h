#ifndef MESSAGESENDERTWO_H
#define MESSAGESENDERTWO_H

#include "WebInterface.h"

using namespace std;

class MessageSenderTwo
{
    public:
        MessageSenderTwo(WebInterface* wi){
            _wi = wi;
            _msg_id = 0;
            _state = STATE_SEND_REQUEST;
            _request_buff = new char[MAX_BUFF_SIZE];
            _response_buff = new char[MAX_BUFF_SIZE];
        }

        ~MessageSenderTwo(){
            delete[] _request_buff;
            delete[] _response_buff;

            _request_buff = nullptr;
            _response_buff = nullptr;
        }

        bool Run(unsigned long forHowLong){
            unsigned long start = millis(); 
            bool result = true;
            while(millis() < forHowLong + start){
                result = result && _state_machine();
            }
            return result;  
        }

    private:
        bool _state_machine(){
            switch(_state){
                case STATE_SEND_REQUEST: 
                {
                    if(_wi->CheckMsgStatus(_msg_id) != _wi->MSG_STATE_ID_NOT_FOUND)
                    {
                        Serial.println("MessageSender: Error - Attempting to send new request when prev request still processing");
                        return false;
                    }
                    else {
                        strcpy(_request_buff, "");
                        sprintf(_request_buff, "POST /post.php HTTP/1.1\r\n");
                        strcat(_request_buff, "Host: posttestserver.com\r\n");
                        strcat(_request_buff, "Connection: Keep-Alive\r\n");
                        strcat(_request_buff, "Content-Length: 31\r\n");
                        strcat(_request_buff, "\r\n");
                        strcat(_request_buff, "{\"param3\":\"three\",\"param4\":\"four\",\"param5\":\"five\"}");
                        strcat(_request_buff, "\r\n\r\n");

                        _msg_id = millis();
                        _wi->SendMessage(
                            _request_buff, _msg_id,
                            _response_buff, MAX_BUFF_SIZE, 5
                        );
                        _state = STATE_RECEIVE_RESPONSE;
                    }
                    break;
                }
                case STATE_RECEIVE_RESPONSE:
                {
                    if(_wi->CheckMsgStatus(_msg_id) == _wi->MSG_STATE_ID_NOT_FOUND)
                    {
                        Serial.println("MessageSender: What, but we just sent a msg!");
                        return false;
                    }
                    else if(_wi->CheckMsgStatus(_msg_id) == _wi->MSG_STATE_RESPONSE_RECEIVED){
                        Serial.println("MessageSender: Response Received: ");
                        Serial.println(_response_buff);
                        _wi->EraseMessageTrace(_msg_id);
                        _state = STATE_RESTART;
                    }
                    else if(_wi->CheckMsgStatus(_msg_id) == _wi->MSG_STATE_SEND_FAILED){
                        Serial.println("MessageSender: Msg failed :(");
                        _wi->EraseMessageTrace(_msg_id);
                        _state = STATE_RESTART;
                    }
                    break;
                }
                case STATE_RESTART: 
                {
                    strcpy(_response_buff, "");
                    _state = STATE_SEND_REQUEST;
                }
            }
        }

    private:
        static const char STATE_SEND_REQUEST = 0;
        static const char STATE_RECEIVE_RESPONSE = 1;
        static const char STATE_RESTART = 2;

    private:
        const short MAX_BUFF_SIZE = 512;

    private:
        WebInterface* _wi;
        unsigned long _msg_id;
        char* _request_buff;
        char* _response_buff;
        char _state;

};
#endif