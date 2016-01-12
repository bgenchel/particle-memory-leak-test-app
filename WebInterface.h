#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#define HOST_NAME "posttestserver.com"
#define HOST_PORT_NUMBER 80

#include <queue>
#include <unordered_map>

using namespace std;

class WebInterface 
{
    public:
        WebInterface(){
            _last_time = 0;
            _num_bytes_received = 0;
            _num_retrys = 0;
            _state = STATE_INIT;
        }

        ~WebInterface(){
        }

        bool Run(unsigned long forHowLong){
           unsigned long start = millis(); 
           bool result = true;
           while(millis() < forHowLong + start){
                result = result && _msg_state_machine();
           }
           return result; 
        }

        bool SendMessage(char* msg, unsigned long returnID, char*& responseBuff, short buffSize, char numRetries)
        {
            //if queue is congested, don't enqueue, return false
            if(_outgoing_request_queue.size() > 10)
            {
                Serial.println("WebInterface: Enqueue failed, too many msgs in queue");
                return false;
            }
            else if(_incoming_responses.count(returnID) > 0)
            {
                Serial.println("WebInterface: returnID already used");
                // delay(30000);
                return false;
            }
            //if queue len and id ok, enqueue msg and return true
            else
            {
                _outgoing_request_queue.push(std::make_tuple(msg, returnID, numRetries));
                _incoming_responses[returnID] = responseBuff;
                _incoming_response_sizes[returnID] = buffSize;
                _msg_status[returnID] = MSG_STATE_BEFORE_SEND;
                return true;
            }
        }

        char CheckMsgStatus(unsigned long ID){
            if(_msg_status.find(ID) != _msg_status.end())
                return _msg_status[ID];
            else
                return MSG_STATE_ID_NOT_FOUND;
        }

        char EraseMessageTrace(unsigned long ID){
            if(_msg_status.find(ID) != _msg_status.end())
            {
                _msg_status.erase(ID);
                if(_incoming_response_sizes.find(ID) != _incoming_response_sizes.end())
                    _incoming_response_sizes.erase(ID);
                return true;
            }
            return false;
        }

        bool TCPConnect(){
            _tcp_client.connect(HOST_NAME, HOST_PORT_NUMBER);
            long start = millis();
            while(!_tcp_client.connected() && (10000 > millis() - start)){
                Serial.println("host connection failed, retrying ... ");
                delay(1000);
                _tcp_client.connect(HOST_NAME, HOST_PORT_NUMBER);
            }

            if(!_tcp_client.connected()){
                return false;
            } 
            return true;
        }

    private:
        bool _msg_state_machine(){
            if(!_tcp_client.connected())
            {
                Serial.println("Disconnected during state machine, retrying...");
                bool rslt = _tcp_client.connect(HOST_NAME, HOST_PORT_NUMBER);
                if(!rslt)
                    return rslt;
            }

            switch(_state){
                case STATE_INIT: {
                    if(_outgoing_request_queue.size() > 0){
                        _curr_msg_id = std::get<1>(_outgoing_request_queue.front());
                        _curr_request_buff = std::get<0>(_outgoing_request_queue.front());
                        _curr_num_retries = std::get<2>(_outgoing_request_queue.front());
                        _curr_response_buff = _incoming_responses[_curr_msg_id];
                        _state = STATE_SEND_MESSAGE;
                    }
                    break;
                }

                case STATE_SEND_MESSAGE: {
                    if(_write_msg(_curr_request_buff))
                    {
                        _state = STATE_RECEIVE_RESPONSE;
                        _msg_status[_curr_msg_id] = MSG_STATE_SENT_WAITING;
                        _last_time = millis();
                    }
                    else
                    {
                        Serial.println("Message send failed, delay then retrying");
                        _num_retrys++;
                        if(_num_retrys > _curr_num_retries){
                            _msg_status[_curr_msg_id] = MSG_STATE_SEND_FAILED;
                            _state = STATE_FINISH_RESTART;
                            delay(2000);
                        }
                    }
                    break;
                }

                case STATE_RECEIVE_RESPONSE: {
                    if(_tcp_client.available() > 0)
                    {
                        //get rid of anything we receive, don't care.
                        _curr_response_buff[_num_bytes_received] = _tcp_client.read();
                        _num_bytes_received++;
                    }
                    else {
                        //terminating null character add
                        _curr_response_buff[_num_bytes_received] = 0;
                        Serial.println("No more bytes to read from server");
                        Serial.print("Number of bytes received = ");
                        Serial.println(_num_bytes_received);
                        Serial.println("Content from server: ");
                        Serial.println(_curr_response_buff);
                        _msg_status[_curr_msg_id] = MSG_STATE_RESPONSE_RECEIVED;

                        _state = STATE_FINISH_RESTART;
                    }
                    break;
                }

                case STATE_FINISH_RESTART: {
                    //pop the current request off the queue
                    if(_outgoing_request_queue.size() > 0)
                        _outgoing_request_queue.pop();
                    else
                        Serial.println("what the hell? how can the queue be 0 if we're at this point.");

                    _num_retrys = 0;
                    _num_bytes_received = 0;
                    // delay(1000);
                    _state = STATE_INIT;
                    break;
                }
            }
            return true;
        }


        bool _write_msg(char*& msg)
        {
            bool rslt = true;
            if(!_tcp_client.connected())
            {
                rslt = _tcp_client.connect(HOST_NAME, HOST_PORT_NUMBER);
            }
            if(rslt == false)
            {
                Serial.println("Can't connect to server!");  
            }
            else {
                Serial.println("Writing message:");
                Serial.println(msg);
                short msg_len = strlen(msg);
                Serial.print("freeMemory before write: ");
                Serial.println(System.freeMemory());
                short bytes_written = _tcp_client.write(msg);
                Serial.print("freeMemory after write: ");
                Serial.println(System.freeMemory());
                rslt = (rslt && bytes_written == msg_len);
                if(bytes_written != msg_len) 
                {
                    Serial.println("Error, bytes_written != msg_len");
                }
            }
            return rslt;
        } 


    public: 
        static const char MSG_STATE_ID_NOT_FOUND = 0;
        static const char MSG_STATE_BEFORE_SEND = 1;
        static const char MSG_STATE_SENT_WAITING = 2;
        static const char MSG_STATE_SEND_FAILED = 3;
        static const char MSG_STATE_RESPONSE_RECEIVED = 4;

    private:
        static const char STATE_INIT = 0;
        static const char STATE_SEND_MESSAGE = 1;
        static const char STATE_RECEIVE_RESPONSE = 2;
        static const char STATE_FINISH_RESTART = 3;

    private:
        queue<tuple<char*, unsigned long, char>>    _outgoing_request_queue;
        unordered_map<unsigned long, char*>         _incoming_responses;
        unordered_map<unsigned long, short>         _incoming_response_sizes;
        unordered_map<unsigned long, char>          _msg_status;
        TCPClient                                   _tcp_client;
        unsigned long                               _last_time;
        unsigned long                               _curr_msg_id;
        char                                        _curr_num_retries;
        char*                                       _curr_request_buff;
        char*                                       _curr_response_buff;
        char                                        _state;
        short                                       _num_retrys;
        short                                       _num_bytes_received;
};

#endif