#include <iostream>
#include <ilcplex/ilocplex.h>

using namespace std;

typedef IloArray<IloNumVarArray> Var2D; // Nos permite definir variaveis de decisão 2D

void printa(string nome, int valor)
{
    cout << nome << ": " << valor << endl;
}

void printa(string nome, int* valores, int tam)
{
    cout << nome << ": ";
    for(int i = 0; i < tam; i++)
    {
        cout << valores[i] << " ";
    }
    cout << endl;
}

int main()
{
    string escolha = "";

    do
    {
        cout << "Digite o numero da opcao de entrada:" << endl;
        cout << "1 - entrada1.txt" << endl;
        cout << "2 - entrada2.txt" << endl;
        cin >> escolha;
    }
    while(escolha != "1" && escolha != "2");

    // Dados do modelo
    int numDePassageiros = 2;
    int numDeVoos = 2;
    int PEN = 100;
    int ATR = 200;
    int MNC = 300;
    int MXC = 400;
    int* AOv = new int[numDeVoos] { 0, 1 };
    int* ADv = new int[numDeVoos] { 2, 3 };
    int* CAPv = new int[numDeVoos] { 4, 5 };
    int* CSTv = new int[numDeVoos] { 6, 7 };
    int* HPv = new int[numDeVoos] { 8, 9 };
    int* HCv = new int[numDeVoos] { 10, 11 };
    int* DESTp = new int[numDePassageiros] { 12, 13 };
    int* PARTp = new int[numDePassageiros] { 14, 15 };
    int* CHEGp = new int[numDePassageiros] { 16, 17 };

    // Printa dados
    printa("numDePassageiros", numDePassageiros);
    printa("numDeVoos", numDeVoos);
    printa("PEN", PEN);
    printa("ATR", ATR);
    printa("MNC", MNC);
    printa("MXC", MXC);
    printa("AOv", AOv, numDeVoos);
    printa("ADv", ADv, numDeVoos);
    printa("CAPv", CAPv, numDeVoos);
    printa("CSTv", CSTv, numDeVoos);
    printa("HPv", HPv, numDeVoos);
    printa("HCv", HCv, numDeVoos);
    printa("DESTp", DESTp, numDePassageiros);
    printa("PARTp", PARTp, numDePassageiros);
    printa("CHEGp", CHEGp, numDePassageiros);
    cout << endl << "Processando..." << endl;

    // Declara o ambiente e o modelo matematico
    IloEnv env;
    IloModel model;
    model = IloModel(env);

    // Variaveis de decisao binarias
    // Variavel booleana: passageiro p foi negligenciado na realocacao?
    IloNumVarArray x(env, numDePassageiros, 0, 1, IloNumVar::Bool);
    for(int p = 0; p < numDePassageiros; p++)
    {
        stringstream var;
        var << "x[" << p << "]";
        x[p].setName(var.str().c_str());
        model.add(x[p]);
    }

    // Variavel booleana de duas dimensoes: passageiro p está alocado ao voo v?
    Var2D y(env, numDePassageiros);

    for(int p = 0; p < numDePassageiros; p++)
    {
        y[p] = IloNumVarArray(env, numDeVoos, 0, 1, IloNumVar::Bool);
    }
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int v = 0; v < numDeVoos; v++)
        {
            stringstream var;
            var << "y[" << p << "][" << v << "]";
            y[p][v].setName(var.str().c_str());
            model.add(y[p][v]);
        }
    }

    // Funcao objetivo (1) ===============================================================
    IloExpr custoRealocacao(env);

    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int v = 0; v < numDeVoos; v++)
        {
            custoRealocacao += (CSTv[v] * y[p][v]);
        }
    }

    IloExpr penalidadeNeglig(env);

    for(int p = 0; p < numDePassageiros; p++)
    {
        penalidadeNeglig += (PEN * x[p]);
    }

    IloExpr passagInsatisfacao(env);

    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int v = 0; v < numDeVoos; v++)
        {
            if(DESTp[p] == ADv[v])
            {
                passagInsatisfacao += (ATR * (HCv[v] - CHEGp[p]) * y[p][v]);
            }
        }
    }
    model.add(IloMinimize(env, custoRealocacao + penalidadeNeglig + passagInsatisfacao));

    // Restricoes =======================================================================
    // Restricao (2) garante que cada passageiro saia do aeroporto de origem ou, então,
    // que seja indicada a sua não realocação
    for(int p = 0; p < numDePassageiros; p++)
    {
        IloExpr expr(env);
        for(int v = 0; v < numDeVoos; v++)
        {
            if (AOv[v] == 0)
            {
                expr += y[p][v];
            }
        }
        expr += x[p];
        IloRange restricao(env, 1, expr, 1);
        model.add(restricao);
    }

    // Restricao (3) impõem a condição que cada passageiro realocado chegue a seu
    // destino final
    for(int p = 0; p < numDePassageiros; p++)
    {
        IloExpr expr(env);
        for(int v = 0; v < numDeVoos; v++)
        {
            if (ADv[v] == DESTp[p])
            {
                expr += y[p][v];
            }
        }
        expr += x[p];
        IloRange restricao(env, 1, expr, 1);
        model.add(restricao);
    }

    // Restricao (4) assegura a continuidade de um itinerário durante as conexões
    // entre aeroportos
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int a = 0; a < numDeVoos; a++)
        {
            if(a != 0 && a != DESTp[p])
            {
                IloExpr expr1(env);
                IloExpr expr2(env);
                for(int v = 0; v < numDeVoos; v++)
                {
                    if(ADv[v] == a)
                    {
                        expr1 += y[p][v];
                    }
                    if(AOv[v] == a)
                    {
                        expr2 += y[p][v];
                    }
                }
                IloRange restricao(env, 0, expr1 - expr2, 0);
                model.add(restricao);
            }
        }
    }

    // Restricao (5) certifica que um voo obedeça sua capacidade máxima
    for(int v = 0; v < numDeVoos; v++)
    {
        IloExpr expr(env);
        for(int p = 0; p < numDePassageiros; p++)
        {
            expr += y[p][v];
        }
        IloRange restricao(env, -IloInfinity, expr, CAPv[v]);
        model.add(restricao);
    }

    // Restricao (6) garante que um passageiro parta do aeroporto de origem somente
    // após a hora de partida do seu itinerário original
    for(int p = 0; p < numDePassageiros; p++)
    {
        IloExpr expr(env);
        for(int v = 0; v < numDeVoos; v++)
        {
            if(AOv[v] == 0)
            {
                expr += (HPv[v] * y[p][v]);
            }
        }
        expr += (PARTp[p] * x[p]);
        IloRange restricao(env, PARTp[p], expr, IloInfinity);
        model.add(restricao);
    }

    // Restricao (7) assegura que o tempo mı́nimo entre conexões seja respeitado
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int a = 0; a < numDeVoos; a++)
        {
            if (a != 0 && a != DESTp[p])
            {
                IloExpr expr1(env);
                IloExpr expr2(env);
                for(int v = 0; v < numDeVoos; v++)
                {
                    if(ADv[v] == a)
                    {
                        expr1 += ((HCv[v] * MNC) * y[p][v]);
                    }
                    if(AOv[v] == a)
                    {
                        expr2 += (HPv[v] * y[p][v]);
                    }
                }
                IloRange restricao(env, -IloInfinity, expr1 / expr2, 1);
                model.add(restricao);
            }
        }
    }

    // Restricao (8) refere-se ao tempo máximo em que uma conexão pode ocorrer
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int a = 0; a < numDeVoos; a++)
        {
            if (a != 0 && a != DESTp[p])
            {
                IloExpr expr1(env);
                IloExpr expr2(env);
                for(int v = 0; v < numDeVoos; v++)
                {
                    if(AOv[v] == a)
                    {
                        expr1 += (HPv[v] * y[p][v]);
                    }
                    if(ADv[v] == a)
                    {
                        expr2 += (HCv[v] * y[p][v]);
                    }
                }
                IloRange restricao(env, -IloInfinity, expr1 - expr2, MXC);
                model.add(restricao);
            }
        }
    }

    // Restricao (9) e (10) evitam a criação de ciclos em um itinerário, garantindo
    // que um passageiro não retorne a um aeroporto já visitado.
    // (9)
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int a = 0; a < numDeVoos; a++)
        {
            IloExpr expr(env);
            for(int v = 0; v < numDeVoos; v++)
            {
                if(AOv[v] == a)
                {
                    expr += y[p][v];
                }
            }
            IloRange restricao(env, -IloInfinity, expr, 1);
            model.add(restricao);
        }
    }
    // (10)
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int a = 0; a < numDeVoos; a++)
        {
            IloExpr expr(env);
            for(int v = 0; v < numDeVoos; v++)
            {
                if(ADv[v] == a)
                {
                    expr += y[p][v];
                }
            }
            IloRange restricao(env, -IloInfinity, expr, 1);
            model.add(restricao);
        }
    }

    // Resolvendo o modelo
    /*
    IloCplex cplex(model);
    cplex.exportModel("ModeloExportado.lp");
    if(!cplex.solve())
    {
        env.error() << "Erro ao rodar modelo!" << endl;
        throw(-1);
    }
    double obj = cplex.getObjValue();
    cout << "Valor da FO: " << obj << endl;

    cout << "Valores de x:" << endl;
    for(int p = 0; p < numDePassageiros; p++)
    {
        double xValue = cplex.getValue(x[p]);
        cout << "\t\t x[" << p << "] = " << xValue << endl;
    }

    cout << "Valores de y:" << endl;
    for(int p = 0; p < numDePassageiros; p++)
    {
        for(int v = 0; v < numDeVoos; v++)
        {
            double yValue = cplex.getValue(y[p][v]);
            cout << "\t\t y[" << p << "][" << v << "] = " << yValue << endl;
        }
    }
    */
    return 0;
}
