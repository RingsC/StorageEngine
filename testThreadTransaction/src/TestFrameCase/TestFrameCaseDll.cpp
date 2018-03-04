#include "TestFrameCase/TestFrameCaseDll.h"
#include "TestFrame/TestFrame.h"

bool Trans1TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int b=12;
	int a=10;
	if (a>b)
	{
		return false;
	}
	return true;
}

bool Trans1TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int b=15;
	int a=12;
	if (a>b)
	{
		return false;
	}
	return true;
}

bool Trans1TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int b=20;
	int a=19;
	if (a>b)
	{
		return false;
	}
	return true;
}

bool Trans1TestFrameCase5(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int b=25;
	int a=24;
	if (a>b)
	{
		return false;
	}
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans1TestFrameCase()
{
	INITRANID()

	ParamBase* pPara = new ParamBase();
	
	REGTASK(Trans1TestFrameCase2,pPara);
	REGTASK(Trans1TestFrameCase3,pPara);
	REGTASK(Trans1TestFrameCase4,pPara);
	REGTASK(Trans1TestFrameCase5,pPara);
	return true;
}


bool Trans2TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=12;
	if ((c-a)>10)
		return false;
	return true;
}

bool Trans2TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=18;
	if ((c-a)>10)
		return false;
	return true;
}

bool Trans2TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=16;
	if ((c-a)>10)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans2TestFrameCase()
{
	INITRANID()
	ParamBase* pPara = new ParamBase();	
	REGTASK(Trans2TestFrameCase2,pPara);
	REGTASK(Trans2TestFrameCase3,pPara);
	REGTASK(Trans2TestFrameCase4,pPara);
	return true;
}


bool Trans3TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c = 20;
	int a = 18;
	if ((c+a)>40)
		return false;
	return true;
}

bool Trans3TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c = 20;
	int a = 19;
	if ((c+a)>40)
		return false;
	return true;
}

bool Trans3TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=12;
	if ((c+a)>40)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans3TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans3TestFrameCase2,pPara);
	REGTASK(Trans3TestFrameCase3,pPara);
	REGTASK(Trans3TestFrameCase4,pPara);
	return true;
}


bool Trans4TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=1;
	if ((c*a)>100)
		return false;
	return true;
}

bool Trans4TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=2;
	if ((c*a)>100)
		return false;
	return true;
}

bool Trans4TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=3;
	if ((c*a)>100)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans4TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans4TestFrameCase2,pPara);
	REGTASK(Trans4TestFrameCase3,pPara);
	REGTASK(Trans4TestFrameCase4,pPara);
	return true;
}

bool Trans5TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=2;
	if ((c/a)>50)
		return false;
	return true;
}

bool Trans5TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=4;
	if ((c/a)>50)
		return false;
	return true;
}

bool Trans5TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=5;
	if ((c/a)>50)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans5TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans5TestFrameCase2,pPara);
	REGTASK(Trans5TestFrameCase3,pPara);
	REGTASK(Trans5TestFrameCase4,pPara);
	return true;
}


bool Trans6TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=12;
	if (c<a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans6TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=15;
	if (c<a)
		return false;
	return true;
}

bool Trans6TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=20;
	int a=15;
	if (c<a)
		return false;
	return true;
}

bool Trans6TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans6TestFrameCase2,pPara);
	REGTASK(Trans6TestFrameCase3,pPara);
	REGTASK(Trans6TestFrameCase4,pPara);
	return true;
}

bool Trans7TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=15;
	int c=16;
	if (c<a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans7TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=15;
	int c=17;
	if (c<a)
		return false;
	return true;
}

bool Trans7TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=15;
	int c=18;
	if (c<a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans7TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans7TestFrameCase2,pPara);
	REGTASK(Trans7TestFrameCase3,pPara);
	REGTASK(Trans7TestFrameCase4,pPara);
	return true;
}

bool Trans8TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=60;
	int a=59;
	if (c<a)
		return false;
	return true;
}

bool Trans8TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=60;
	int a=58;
	if (c<a)
		return false;;
	return true;
}

bool Trans8TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int c=60;
	int a=57;
	if (c<a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans8TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans8TestFrameCase2,pPara);
	REGTASK(Trans8TestFrameCase3,pPara);
	REGTASK(Trans8TestFrameCase4,pPara);
	return true;
}

bool Trans9TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=100;
	int c=99;
	if (c>a)
		return false;
	return true;
}

bool Trans9TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=100;
	int c=98;
	if (c>a)
		return false;
	return true;
}

bool Trans9TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=100;
	int c=97;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;

}

bool Trans9TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans9TestFrameCase2,pPara);
	REGTASK(Trans9TestFrameCase3,pPara);
	REGTASK(Trans9TestFrameCase4,pPara);
	return true;
}

bool Trans10TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=100;
	int c=96;
	if (c>a)
		return false;
	return true;
}

bool Trans10TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a=100;
	int c=95;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans10TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c=94;
	if (c>a)
		return false;
	return true;
}

bool Trans10TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans10TestFrameCase2,pPara);
	REGTASK(Trans10TestFrameCase3,pPara);
	REGTASK(Trans10TestFrameCase4,pPara);
	return true;
}

bool Trans11TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c=93;
	if (c>a)
		return false;
	return true;
}

bool Trans11TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c=92;
	if (c>a)
		return false;
	return true;
}

bool Trans11TestFrameCase4(ParamBase* arg)
{ 
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 91;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans11TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans11TestFrameCase2,pPara);
	REGTASK(Trans11TestFrameCase3,pPara);
	REGTASK(Trans11TestFrameCase4,pPara);
	return true;
}

bool Trans12TestFrameCase2(ParamBase* arg)
{ 
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 90;
	if (c>a)
		return false;
	return true;
}

bool Trans12TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 89;
	if (c>a)
		return false;
	return true;
}

bool Trans12TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 88;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans12TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans12TestFrameCase2,pPara);
	REGTASK(Trans12TestFrameCase3,pPara);
	REGTASK(Trans12TestFrameCase4,pPara);
	return true;
}

bool Trans13TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 87;
	if (c>a)
		return false;
	return true;
}

bool Trans13TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 86;
	if (c>a)
		return false;
	return true;
}

bool Trans13TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 85;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans13TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans13TestFrameCase2,pPara);
	REGTASK(Trans13TestFrameCase3,pPara);
	REGTASK(Trans13TestFrameCase4,pPara);
	return true;
}

bool Trans14TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 84;
	if (c>a)
		return false;
	return true;
}

bool Trans14TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 83;
	if (c>a)
		return false;
	return true;
}

bool Trans14TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 82;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans14TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans14TestFrameCase2,pPara);
	REGTASK(Trans14TestFrameCase3,pPara);
	REGTASK(Trans14TestFrameCase4,pPara);
	return true;
}

bool Trans15TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 36;
	if (c>a)
		return false;
	return true;
}

bool Trans15TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 81;
	if (c>a)
		return false;
	return true;
}

bool Trans15TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 80;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans15TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans15TestFrameCase2,pPara);
	REGTASK(Trans15TestFrameCase3,pPara);
	REGTASK(Trans15TestFrameCase4,pPara);
	return true;
}

bool Trans16TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 79;
	if (c>a)
		return false;
	return true;
}

bool Trans16TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 78;
	if (c>a)
		return false;
	return true;
}

bool Trans16TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 77;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans16TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans16TestFrameCase2,pPara);
	REGTASK(Trans16TestFrameCase3,pPara);
	REGTASK(Trans16TestFrameCase4,pPara);
	return true;
}

bool Trans17TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 75;
	if (c>a)
		return false;
	return true;
}

bool Trans17TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 76;
	if (c>a)
		return false;
	return true;
}

bool Trans17TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 74;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans17TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans17TestFrameCase2,pPara);
	REGTASK(Trans17TestFrameCase3,pPara);
	REGTASK(Trans17TestFrameCase4,pPara);
	return true;
}

bool Trans18TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 73;
	if (c>a)
		return false;
	return true;
}

bool Trans18TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 72;
	if (c>a)
		return false;
	return true;
}

bool Trans18TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 71;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans18TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans18TestFrameCase2,pPara);
	REGTASK(Trans18TestFrameCase3,pPara);
	REGTASK(Trans18TestFrameCase4,pPara);
	return true;
}

bool Trans19TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 70;
	if (c>a)
		return false;
	return true;
}

bool Trans19TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 69;
	if (c>a)
		return false;
	return true;
}

bool Trans19TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 68;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans19TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans19TestFrameCase2,pPara);
	REGTASK(Trans19TestFrameCase3,pPara);
	REGTASK(Trans19TestFrameCase4,pPara);
	return true;
}

bool Trans20TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 67;
	if (c>a)
		return false;
	return true;
}

bool Trans20TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 66;
	if (c>a)
		return false;
	return true;
}

bool Trans20TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 65;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans20TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans20TestFrameCase2,pPara);
	REGTASK(Trans20TestFrameCase3,pPara);
	REGTASK(Trans20TestFrameCase4,pPara);
	return true;
}

bool Trans21TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 64;
	if (c>a)
		return false;
	return true;
}

bool Trans21TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 63;
	if (c>a)
		return false;
	return true;
}

bool Trans21TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 62;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans21TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans21TestFrameCase2,pPara);
	REGTASK(Trans21TestFrameCase3,pPara);
	REGTASK(Trans21TestFrameCase4,pPara);
	return true;

}

bool Trans22TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 61;
	if (c>a)
		return false;
	return true;
}

bool Trans22TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 37;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans22TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 60;
	if (c>a)
		return false;
	return true;
}

bool Trans22TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans22TestFrameCase2,pPara);
	REGTASK(Trans22TestFrameCase3,pPara);
	REGTASK(Trans22TestFrameCase4,pPara);
	return true;

}

bool Trans23TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 59;
	if (c>a)
		return false;
	return true;
}

bool Trans23TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 58;
	if (c>a)
		return false;
	return true;
}

bool Trans23TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 57;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans23TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans23TestFrameCase2,pPara);
	REGTASK(Trans23TestFrameCase3,pPara);
	REGTASK(Trans23TestFrameCase4,pPara);
	return true;
}

bool Trans24TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 56;
	if (c>a)
		return false;
	return true;
}

bool Trans24TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 56;
	if (c>a)
		return false;
	return true;
}

bool Trans24TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 55;
	if (c>a)
		return false;
	arg->SetSuccessFlag(false);
	return true;
}

bool Trans24TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans24TestFrameCase2,pPara);
	REGTASK(Trans24TestFrameCase3,pPara);
	REGTASK(Trans24TestFrameCase4,pPara);
	return true;

}

bool Trans25TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 54;
	if (c>a)
		return false;
	return true;
}

bool Trans25TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 53;
	if (c>a)
		return false;
	return true;
}

bool Trans25TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 52;
	if (c>a)
		return false;
	return true;
}

bool Trans25TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans25TestFrameCase2,pPara);
	REGTASK(Trans25TestFrameCase3,pPara);
	REGTASK(Trans25TestFrameCase4,pPara);
	return true;
}

bool Trans26TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 51;
	if (c>a)
		return false;
	return true;
}

bool Trans26TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 50;
	if (c>a)
		return false;
	return true;
}

bool Trans26TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 49;
	if (c>a)
		return false;
	return true;
}

bool Trans26TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans26TestFrameCase2,pPara);
	REGTASK(Trans26TestFrameCase3,pPara);
	REGTASK(Trans26TestFrameCase4,pPara);
	return true;
}

bool Trans27TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 48;
	if (c>a)
		return false;
	return true;
}

bool Trans27TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 48;
	if (c>a)
		return false;
	return true;
}

bool Trans27TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 47;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans27TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans27TestFrameCase2,pPara);
	REGTASK(Trans27TestFrameCase3,pPara);
	REGTASK(Trans27TestFrameCase4,pPara);
	return true;
}

bool Trans28TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 46;
	if (c>a)
		return false;
	return true;
}

bool Trans28TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 45;
	if (c>a)
		return false;
	return true;
}

bool Trans28TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 44;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans28TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans28TestFrameCase2,pPara);
	REGTASK(Trans28TestFrameCase3,pPara);
	REGTASK(Trans28TestFrameCase4,pPara);
	return true;
}

bool Trans29TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 43;
	if (c>a)
		return false;
	return true;
}


bool Trans29TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 42;
	if (c>a)
		return false;
	return true;
}

bool Trans29TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 41;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans29TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans29TestFrameCase2,pPara);
	REGTASK(Trans29TestFrameCase3,pPara);
	REGTASK(Trans29TestFrameCase4,pPara);
	return true;
}

bool Trans30TestFrameCase2(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 40;
	if (c>a)
		return false;
	return true;
}

bool Trans30TestFrameCase3(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 39;
	if (c>a)
		return false;
	return true;
}

bool Trans30TestFrameCase4(ParamBase* arg)
{
	printf("\n--------%s--------\n",__FUNCTION__);
	int a = 100;
	int c = 38;
	if (c>a)
		return false;
	arg->SetSuccessFlag(true);
	return true;
}

bool Trans30TestFrameCase()
{
	INITRANID()
		
	ParamBase* pPara = new ParamBase();
	REGTASK(Trans30TestFrameCase2,pPara);
	REGTASK(Trans30TestFrameCase3,pPara);
	REGTASK(Trans30TestFrameCase4,pPara);
	return true;
}

