# Projeto SO

O trabalho a desenvolver devesimular um ambiente simplificado de edge computing onde vários clientes podem delega(fazer offload) tarefas para nós (servidores) que se encontram geograficamente próximos do cliente. Para distribuir os pedidos dos clientes pelos recursos existentes é necessário um sistema que mantenha informação sobre os recursos disponíveis e a sua utilização, que receba os pedidos com as tarefas que os clientes precisam de ver satisfeitas e que distribua essas tarefas pelos vários servidores.

- `Mobile Node` -> Processo que gera tarefas para offloading e as envia pelo named pipe. Podem existir vários destes processos.
- `System Manager` -> Processo responsável por iniciar o sistema, ler o ficheiro de configuração e criar os restantes processos envolvidos na execução de tarefas.
- `Task Manager` -> Processo responsável por gerir a receção de tarefas e a sua distribuição pelos processos Edge Server.
- `Edge Server` -> Processo responsável pela execução das tarefas. Dentro de cada Edge Server existem 2 vCPU (virtual Central Processing Unit) que são utilizados para executar as tarefas. Podem existir vários destes processos.
- `Monitor` -> Processo responsável por controlar o número de vCPUs ativos dentro dos processos Edge Server.
- `Maintenance Manager` -> Processo responsável pela manutenção dos servidores Edge Server.

## Desenvolvedores

### Eduardo Figueiredo
### Fábio Santos
