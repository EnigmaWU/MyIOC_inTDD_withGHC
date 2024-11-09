# About
* This docuemet is IOC's Glossary.

# Glossary
## ModMgr vs ModUsr
* ModMgr: Module Manager.
* ModUsr: Module User.

## MSG: CMD vs EVT vs DAT
* MSG: Message.
    * CMD: Command.
    * EVT: Event.
    * DAT: Data.

## EvtProducer vs EvtConsumer
* EvtProducer: Event Producer（事件生产者）.
* EvtConsumer: Event Consumer（事件消费者）.

## CmdInitiator vs CmdExecutor
* CmdInitiator: Command Initiator（命令发起者）.
* CmdExecutor: Command Executor（命令执行者）.

## DatSender vs DatReceiver
* DatSender: Data Sender（数据发送者）.
* DatReceiver: Data Receiver（数据接收者）.

## Service vs Link
* Service: online/accept new incoming Links on server side.
* Link: connect to Service on client side.
* Each accept/connect will establish a pair Link, and take communication of CMD/EVT/DAT in `Half-Duplex` mode:
  * which means the pair link MUST SELECT ONE USAGE of CMD/EVT/DAT, either from client to server or from server to client.

## Conet vs Conles
* Conet: Connect Mode, explicit connect and disconnect to Service.
* Conles: Connectless Mode, implicit auto established Links.