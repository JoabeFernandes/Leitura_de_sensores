# Leitura_de_sensores

A aplicação a ser projetada receberá dados de uma rede de sensores através de uma ISR (Rotina de Tratamento de Interrupção) que disponibilizará os dados através de uma fila de mensagens. Os dados são compostos por dois campos: um campo de identificação, que contém um caractere que identifica o sensor; e um campo valor, que contém o valor medido pelo sensor. Os dados dos sensores devem ser processados de forma independente (por tarefas diferentes). O processamento consiste em realizar a média a cada período de 10s e apresentar o resultado. A leitura do sensor A tem uma periodicidade de 50 ms, a leitura do sensor B tem uma periodicidade de 100 ms e  a leitura do sensor C tem uma periodicidade de 1s.
Implementação utilizando o FreeRTOS
