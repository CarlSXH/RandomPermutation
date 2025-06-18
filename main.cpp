

// Animation2D.cpp : Defines the entry point for the application.
//

#include "DynamicBitvector.h"
#include <fstream>
#include <vector>
#include <windows.h>

#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>

#pragma comment(lib, "D2D1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "Dxguid.lib")
#pragma comment(lib, "Windowscodecs.lib")


#include <wincodec.h>
#include <string>
#include <vector>
#include <ctime>


#define _USE_MATH_DEFINES
#include <math.h>

#include "RandPerm.h"
#include <chrono>





#ifndef _UNICODE
typedef CHAR Char;
#define Text(t) (t)
#else
typedef WCHAR Char;
#define Text(t) L##t
#endif

using namespace D2D1;
using namespace std;

// Global Variables:
HINSTANCE hInst;                                // current instance
const Char *szTitle;                  // The title bar text
const Char *szWindowClass;            // the main window class name
HWND  g_hWnd;


ID2D1Factory1 *g_pFactory = NULL;
IDWriteFactory *g_pWriteFactory = NULL;
IWICImagingFactory *g_pWICImageingFactory = NULL;

IDWriteTextFormat *g_pTextFormat = NULL;


int g_nWidth = 700;
int g_nHeight = 700;

int s = 1;

bool forProf = false;


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

HRESULT             InitD2D();
void                CleanUpD2D();

HRESULT InitD2D()
{
    HRESULT hr;
    CoInitialize(NULL);
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        (LPVOID *)&g_pWICImageingFactory
    );
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pFactory);
    if (FAILED(hr))
        return hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE::DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)&g_pWriteFactory);
    hr = g_pWriteFactory->CreateTextFormat(L"Microsoft Sans Serif", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"", &g_pTextFormat);
    g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);


    return hr;
}
void CleanUpD2D()
{
    if (g_pFactory != NULL)
    {
        g_pFactory->Release();
        g_pFactory = NULL;
    }
    if (g_pTextFormat != NULL)
    {
        g_pTextFormat->Release();
        g_pTextFormat = NULL;
    }
    if (g_pWriteFactory != NULL)
    {
        g_pWriteFactory->Release();
        g_pWriteFactory = NULL;
    }
    if (g_pWICImageingFactory != NULL)
    {
        g_pWICImageingFactory->Release();
        g_pWICImageingFactory = NULL;
    }

    CoUninitialize();
}

void Refresh(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    InvalidateRect(hWnd, &rect, TRUE);
    UpdateWindow(hWnd);
}

int g_nX = 0;
int g_nY = 0;


MonoPermData monotoneRestriction;

int RestrictionK;
PermData restricion;


int N = 10000;
unsigned int *Perm;
unsigned int *PermHasting;
unsigned int *PermDirect;
RandDevice device;

// space on the side of the slider
int SliderBorder = 30;
// height of slider
int SliderHeight = 30;
bool isOnSlider = false;
// radius of the slider nod
int SliderR = 6;
float q = 1;

float SliderLeft = -100;
float SliderRight = 100;

float SliderValue = 0.5f;
float r = 0;

FenwickTree *g_pFT;

int Count = 0;
float avg1 = 0;
float avg2 = 0;

int drawCount = 0;
float avgDraw = 0;

float ShuffleInterval = 0.001;
float ResampleInterval = 0.3;
float RepeatInterval = 0.01;
bool DoRepeat = false;

int *densityArray = NULL;
int *colorArray = NULL;
int splitCount;

bool displayOption = false;


bool isDirectSample = true;

int ShuffleCount = 0;
float ShuffleTime = 0;

int ShuffleSuccess = 0;

bool hastingValid = false;

bool metropolisAccept = false;

bool showRestriction = true;


bool ValidRestriction = true;


enum TextOptions {
    None = 0,
    Essential = 1,
    Verbose = 2,
};
TextOptions textOption = Verbose;


int intensityToRgb(float intensity) {
    float colorTable[] = {0.1509, 0.0815, 0.5385                       ,
        0.15170470588235294, 0.0828835294117647, 0.5461376470588235    ,
        0.1525094117647059, 0.08426705882352942, 0.553775294117647     ,
        0.15331411764705882, 0.08565058823529412, 0.5614129411764706   ,
        0.15411882352941175, 0.08703411764705883, 0.5690505882352941   ,
        0.1549235294117647, 0.08841764705882353, 0.5766882352941176    ,
        0.15572823529411764, 0.08980117647058825, 0.5843258823529411   ,
        0.15653294117647057, 0.09118470588235295, 0.5919635294117647   ,
        0.1590070588235294, 0.0933964705882353, 0.6002482352941176     ,
        0.1616329411764706, 0.09568352941176471, 0.6085917647058824    ,
        0.16425882352941176, 0.09797058823529411, 0.616935294117647    ,
        0.16688470588235294, 0.10025764705882353, 0.6252788235294118   ,
        0.1695105882352941, 0.10254470588235294, 0.6336223529411764    ,
        0.17213647058823528, 0.10483176470588235, 0.6419658823529412   ,
        0.17476235294117648, 0.10711882352941177, 0.6503094117647058   ,
        0.17858823529411763, 0.11028823529411765, 0.658535294117647    ,
        0.18265411764705883, 0.11363411764705883, 0.6667376470588235   ,
        0.18672, 0.11698, 0.67494                                      ,
        0.19078588235294117, 0.12032588235294119, 0.6831423529411764   ,
        0.19485176470588234, 0.12367176470588237, 0.691344705882353    ,
        0.19891764705882353, 0.12701764705882354, 0.6995470588235294   ,
        0.2029835294117647, 0.1303635294117647, 0.7077494117647058     ,
        0.20816117647058824, 0.13446117647058825, 0.714755294117647    ,
        0.21370941176470587, 0.1388094117647059, 0.7213623529411765    ,
        0.21925764705882353, 0.14315764705882356, 0.7279694117647059   ,
        0.22480588235294116, 0.14750588235294118, 0.7345764705882353   ,
        0.23035411764705882, 0.15185411764705883, 0.7411835294117648   ,
        0.23590235294117645, 0.1562023529411765, 0.7477905882352942    ,
        0.2414505882352941, 0.1605505882352941, 0.7543976470588236     ,
        0.24802470588235292, 0.16550117647058823, 0.7604494117647059   ,
        0.25511176470588237, 0.17075294117647058, 0.7662235294117647   ,
        0.26219882352941176, 0.17600470588235295, 0.7719976470588236   ,
        0.26928588235294115, 0.1812564705882353, 0.7777717647058824    ,
        0.27637294117647054, 0.1865082352941176, 0.7835458823529412    ,
        0.28346, 0.19175999999999999, 0.78932                          ,
        0.2905470588235294, 0.19701176470588236, 0.7950941176470588    ,
        0.2985482352941176, 0.20276588235294116, 0.8003082352941177    ,
        0.3072023529411764, 0.20887882352941173, 0.8051223529411764    ,
        0.3158564705882353, 0.21499176470588233, 0.8099364705882353    ,
        0.32451058823529416, 0.22110470588235295, 0.8147505882352941   ,
        0.3331647058823529, 0.22721764705882352, 0.819564705882353     ,
        0.34181882352941173, 0.2333305882352941, 0.8243788235294117    ,
        0.3504729411764706, 0.23944352941176472, 0.8291929411764706    ,
        0.3598258823529412, 0.24588117647058827, 0.8334                ,
        0.36987764705882353, 0.2526435294117647, 0.837                 ,
        0.3799294117647059, 0.25940588235294115, 0.8406                ,
        0.3899811764705883, 0.26616823529411765, 0.8442                ,
        0.40003294117647065, 0.27293058823529415, 0.8478               ,
        0.41008470588235296, 0.2796929411764706, 0.8514                ,
        0.42013647058823533, 0.28645529411764703, 0.855                ,
        0.4306882352941177, 0.29347647058823534, 0.858                 ,
        0.4419400000000001, 0.30086000000000007, 0.86016               ,
        0.4531917647058824, 0.30824352941176475, 0.86232               ,
        0.4644435294117647, 0.3156270588235294, 0.8644799999999999     ,
        0.4756952941176471, 0.32301058823529416, 0.86664               ,
        0.4869470588235295, 0.3303941176470589, 0.8688                 ,
        0.4981988235294118, 0.33777764705882357, 0.87096               ,
        0.5096811764705883, 0.34530235294117645, 0.8726070588235294    ,
        0.521624705882353, 0.3531094117647059, 0.8732282352941176      ,
        0.5335682352941178, 0.36091647058823534, 0.8738494117647059    ,
        0.5455117647058824, 0.3687235294117647, 0.8744705882352941     ,
        0.557455294117647, 0.3765305882352941, 0.8750917647058823      ,
        0.5693988235294118, 0.38433764705882356, 0.8757129411764706    ,
        0.5813423529411765, 0.392144705882353, 0.8763341176470588      ,
        0.5933670588235295, 0.39998, 0.8765705882352941                ,
        0.605635294117647, 0.4079, 0.8756529411764706                  ,
        0.6179035294117646, 0.41581999999999997, 0.874735294117647     ,
        0.6301717647058824, 0.42374, 0.8738176470588235                ,
        0.64244, 0.43166, 0.8729                                       ,
        0.6547082352941176, 0.43957999999999997, 0.8719823529411764    ,
        0.6669764705882354, 0.4475, 0.8710647058823529                 ,
        0.6792400000000001, 0.45539882352941174, 0.8699635294117647    ,
        0.69148, 0.46319176470588236, 0.867944705882353                ,
        0.70372, 0.4709847058823529, 0.8659258823529411                ,
        0.7159599999999999, 0.4787776470588235, 0.8639070588235294     ,
        0.7282000000000001, 0.48657058823529414, 0.8618882352941176    ,
        0.7404400000000001, 0.4943635294117647, 0.8598694117647059     ,
        0.75268, 0.5021564705882353, 0.8578505882352941                ,
        0.7648623529411767, 0.5099282352941177, 0.8557529411764705     ,
        0.7764105882352943, 0.5174670588235295, 0.8527882352941176     ,
        0.7879588235294118, 0.5250058823529412, 0.8498235294117646     ,
        0.7995070588235295, 0.5325447058823529, 0.8468588235294118     ,
        0.8110552941176471, 0.5400835294117646, 0.8438941176470588     ,
        0.8226035294117648, 0.5476223529411766, 0.8409294117647058     ,
        0.8341517647058824, 0.5551611764705883, 0.837964705882353      ,
        0.8457, 0.5627, 0.835                                          ,
        0.8559776470588236, 0.5697870588235294, 0.8311882352941176     ,
        0.8662552941176471, 0.5768741176470589, 0.8273764705882353     ,
        0.8765329411764706, 0.5839611764705882, 0.823564705882353      ,
        0.8868105882352941, 0.5910482352941177, 0.8197529411764706     ,
        0.8970882352941176, 0.598135294117647, 0.8159411764705883      ,
        0.9073658823529412, 0.6052223529411765, 0.8121294117647059     ,
        0.9176435294117646, 0.6123094117647059, 0.8083176470588236     ,
        0.9264070588235294, 0.6188270588235294, 0.8040400000000001     ,
        0.9350329411764706, 0.6252929411764706, 0.79972                ,
        0.9436588235294119, 0.6317588235294118, 0.7954                 ,
        0.952284705882353, 0.6382247058823529, 0.79108                 ,
        0.9609105882352942, 0.6446905882352941, 0.78676                ,
        0.9695364705882353, 0.6511564705882352, 0.78244                ,
        0.9781623529411766, 0.6576223529411765, 0.7781199999999999     ,
        0.9819294117647059, 0.6635352941176471, 0.7737411764705882     ,
        0.984724705882353, 0.6693376470588235, 0.769350588235294       ,
        0.9875200000000001, 0.6751400000000001, 0.7649599999999999     ,
        0.9903152941176471, 0.6809423529411766, 0.7605694117647058     ,
        0.9931105882352941, 0.6867447058823529, 0.7561788235294117     ,
        0.9959058823529412, 0.6925470588235294, 0.7517882352941176     ,
        0.9987011764705882, 0.6983494117647059, 0.7473976470588235     ,
        0.999410588235294, 0.70376, 0.7432399999999999                 ,
        0.9994247058823529, 0.70904, 0.7391599999999999                ,
        0.9994388235294117, 0.71432, 0.73508                           ,
        0.9994529411764705, 0.7196, 0.731                              ,
        0.9994670588235295, 0.72488, 0.72692                           ,
        0.9994811764705883, 0.73016, 0.72284                           ,
        0.9994952941176471, 0.73544, 0.7187600000000001                ,
        0.9987941176470588, 0.7404, 0.7149529411764707                 ,
        0.997735294117647, 0.7452, 0.7112823529411765                  ,
        0.9966764705882353, 0.75, 0.7076117647058824                   ,
        0.9956176470588236, 0.7548, 0.7039411764705883                 ,
        0.9945588235294117, 0.7596, 0.7002705882352941                 ,
        0.9935, 0.7644, 0.6966                                         ,
        0.9924411764705883, 0.7692, 0.6929294117647059                 ,
        0.9909705882352942, 0.7737364705882352, 0.6894317647058824     ,
        0.9892058823529412, 0.7780847058823529, 0.6860576470588235     ,
        0.9874411764705883, 0.7824329411764707, 0.6826835294117647     ,
        0.9856764705882353, 0.7867811764705882, 0.6793094117647058     ,
        0.9839117647058824, 0.7911294117647059, 0.6759352941176471     ,
        0.9821470588235295, 0.7954776470588236, 0.6725611764705882     ,
        0.9803823529411765, 0.7998258823529413, 0.6691870588235294     ,
        0.9783352941176471, 0.8039835294117648, 0.665975294117647      ,
        0.9760058823529412, 0.8079505882352941, 0.6629258823529411     ,
        0.9736764705882354, 0.8119176470588235, 0.6598764705882353     ,
        0.9713470588235295, 0.8158847058823528, 0.6568270588235294     ,
        0.9690176470588235, 0.8198517647058823, 0.6537776470588236     ,
        0.9666882352941176, 0.8238188235294117, 0.6507282352941176     ,
        0.9643588235294117, 0.8277858823529411, 0.6476788235294118     ,
        0.9618235294117646, 0.8316117647058823, 0.6447411764705883     ,
        0.959, 0.83524, 0.6419600000000001                             ,
        0.9561764705882353, 0.8388682352941176, 0.6391788235294118     ,
        0.9533529411764706, 0.8424964705882352, 0.6363976470588236     ,
        0.9505294117647058, 0.8461247058823529, 0.6336164705882353     ,
        0.9477058823529411, 0.8497529411764706, 0.630835294117647      ,
        0.9448823529411764, 0.8533811764705882, 0.6280541176470588     ,
        0.941955294117647, 0.8568964705882353, 0.6253576470588235      ,
        0.9388211764705882, 0.8601858823529411, 0.6228305882352941     ,
        0.9356870588235293, 0.863475294117647, 0.6203035294117647      ,
        0.9325529411764706, 0.866764705882353, 0.6177764705882353      ,
        0.9294188235294117, 0.8700541176470589, 0.6152494117647058     ,
        0.9262847058823529, 0.8733435294117647, 0.6127223529411764     ,
        0.9231505882352941, 0.8766329411764706, 0.6101952941176471     ,
        0.9199741176470587, 0.8798552941176472, 0.6077141176470587     ,
        0.916670588235294, 0.8828764705882354, 0.605370588235294       ,
        0.9133670588235293, 0.8858976470588236, 0.6030270588235294     ,
        0.9100635294117646, 0.8889188235294118, 0.6006835294117646     ,
        0.9067599999999999, 0.8919400000000001, 0.59834                ,
        0.9034564705882352, 0.8949611764705883, 0.5959964705882352     ,
        0.9001529411764706, 0.8979823529411765, 0.5936529411764706     ,
        0.8968329411764705, 0.9009588235294118, 0.5913376470588235     ,
        0.893430588235294, 0.9037117647058824, 0.5891635294117646      ,
        0.8900282352941176, 0.906464705882353, 0.5869894117647059      ,
        0.886625882352941, 0.9092176470588236, 0.584815294117647       ,
        0.8832235294117646, 0.9119705882352942, 0.5826411764705882     ,
        0.8798211764705882, 0.9147235294117647, 0.5804670588235294     ,
        0.8764188235294117, 0.9174764705882353, 0.5782929411764706     ,
        0.8730164705882353, 0.9202105882352941, 0.576124705882353      ,
        0.8696141176470588, 0.9227376470588236, 0.5740211764705883     ,
        0.8662117647058822, 0.925264705882353, 0.5719176470588235      ,
        0.8628094117647058, 0.9277917647058824, 0.5698141176470588     ,
        0.8594070588235293, 0.9303188235294118, 0.5677105882352941     ,
        0.8560047058823529, 0.9328458823529412, 0.5656070588235295     ,
        0.8526023529411764, 0.9353729411764705, 0.5635035294117647     ,
        0.8492, 0.9379, 0.5614                                         ,
        0.8458541176470588, 0.9402294117647059, 0.5594376470588236     ,
        0.8425082352941176, 0.9425588235294118, 0.5574752941176471     ,
        0.8391623529411764, 0.9448882352941177, 0.5555129411764705     ,
        0.8358164705882353, 0.9472176470588236, 0.5535505882352941     ,
        0.8324705882352941, 0.9495470588235294, 0.5515882352941176     ,
        0.8291247058823529, 0.9518764705882353, 0.5496258823529412     ,
        0.8257788235294118, 0.9542058823529412, 0.5476635294117647     ,
        0.8224976470588236, 0.9563670588235295, 0.5458176470588235     ,
        0.8192223529411765, 0.9585129411764706, 0.5439823529411765     ,
        0.8159470588235295, 0.9606588235294118, 0.5421470588235294     ,
        0.8126717647058823, 0.962804705882353, 0.5403117647058823      ,
        0.8093964705882353, 0.9649505882352941, 0.5384764705882352     ,
        0.8061211764705882, 0.9670964705882353, 0.5366411764705882     ,
        0.8028458823529412, 0.9692423529411764, 0.5348058823529411     ,
        0.7996294117647059, 0.9712352941176471, 0.5330764705882353     ,
        0.7964247058823529, 0.9731976470588235, 0.5313682352941176     ,
        0.79322, 0.97516, 0.52966                                      ,
        0.790015294117647, 0.9771223529411766, 0.5279517647058823      ,
        0.786810588235294, 0.979084705882353, 0.5262435294117647       ,
        0.7836058823529412, 0.9810470588235295, 0.524535294117647      ,
        0.7804011764705882, 0.9830094117647059, 0.5228270588235294     ,
        0.777270588235294, 0.984844705882353, 0.5211823529411764       ,
        0.774164705882353, 0.9866376470588235, 0.5195588235294117      ,
        0.7710588235294118, 0.9884305882352942, 0.517935294117647      ,
        0.7679529411764706, 0.9902235294117647, 0.5163117647058824     ,
        0.7648470588235294, 0.9920164705882353, 0.5146882352941177     ,
        0.7617411764705881, 0.9938094117647059, 0.5130647058823529     ,
        0.7586352941176471, 0.9956023529411765, 0.5114411764705882     ,
        0.755595294117647, 0.9965576470588235, 0.5098741176470588      ,
        0.7525882352941176, 0.9970941176470588, 0.508335294117647      ,
        0.7495811764705882, 0.9976305882352942, 0.5067964705882353     ,
        0.7465741176470588, 0.9981670588235294, 0.5052576470588235     ,
        0.7435670588235294, 0.9987035294117647, 0.5037188235294118     ,
        0.7405599999999999, 0.99924, 0.50218                           ,
        0.7375529411764704, 0.9997764705882353, 0.5006411764705881     ,
        0.7346035294117645, 1.0, 0.4991517647058823                    ,
        0.731695294117647, 1.0, 0.4976976470588235                     ,
        0.7287870588235293, 1.0, 0.4962435294117647                    ,
        0.7258788235294117, 1.0, 0.49478941176470587                   ,
        0.722970588235294, 1.0, 0.49333529411764704                    ,
        0.7200623529411765, 1.0, 0.49188117647058827                   ,
        0.7171541176470588, 1.0, 0.49042705882352944                   ,
        0.7143023529411764, 1.0, 0.48900823529411763                   ,
        0.7115070588235294, 1.0, 0.4876247058823529                    ,
        0.7087117647058823, 1.0, 0.48624117647058823                   ,
        0.7059164705882353, 1.0, 0.4848576470588235                    ,
        0.7031211764705881, 1.0, 0.48347411764705883                   ,
        0.7003258823529411, 1.0, 0.4820905882352941                    ,
        0.6975305882352941, 1.0, 0.4807070588235294                    ,
        0.694770588235294, 1.0, 0.4793647058823529                     ,
        0.6920599999999999, 1.0, 0.47807999999999995                   ,
        0.6893494117647058, 1.0, 0.47679529411764704                   ,
        0.6866388235294116, 1.0, 0.4755105882352941                    ,
        0.6839282352941176, 1.0, 0.47422588235294116                   ,
        0.6812176470588235, 1.0, 0.4729411764705882                    ,
        0.6785070588235294, 1.0, 0.4716564705882353                    ,
        0.6758341176470588, 1.0, 0.47038588235294115                   ,
        0.6732364705882353, 1.0, 0.46914352941176474                   ,
        0.6706388235294117, 1.0, 0.4679011764705882                    ,
        0.6680411764705881, 1.0, 0.46665882352941174                   ,
        0.6654435294117647, 1.0, 0.4654164705882353                    ,
        0.6628458823529412, 1.0, 0.46417411764705885                   ,
        0.6602482352941176, 1.0, 0.4629317647058824                    ,
        0.6576717647058824, 0.9997988235294117, 0.4617                 ,
        0.6551588235294118, 0.9989941176470588, 0.4605                 ,
        0.6526458823529411, 0.9981894117647059, 0.4593                 ,
        0.6501329411764706, 0.997384705882353, 0.4581                  ,
        0.64762, 0.99658, 0.45690000000000003                          ,
        0.6451070588235294, 0.995775294117647, 0.4557                  ,
        0.6425941176470588, 0.994970588235294, 0.4545                  ,
        0.640095294117647, 0.9940435294117647, 0.4533023529411765      ,
        0.6376670588235294, 0.992504705882353, 0.4521164705882353      ,
        0.6352388235294117, 0.9909658823529411, 0.45093058823529414    ,
        0.6328105882352941, 0.9894270588235294, 0.44974470588235294    ,
        0.6303823529411764, 0.9878882352941176, 0.44855882352941173    ,
        0.6279541176470588, 0.9863494117647059, 0.4473729411764706     ,
        0.6255258823529412, 0.9848105882352941, 0.4461870588235294     ,
        0.6231035294117647, 0.9832376470588235, 0.4450058823529412     ,
        0.6207458823529411, 0.981289411764706, 0.4438764705882353      ,
        0.6183882352941177, 0.9793411764705883, 0.44274705882352944    ,
        0.6160305882352941, 0.9773929411764707, 0.44161764705882356    ,
        0.6136729411764705, 0.975444705882353, 0.4404882352941176      ,
        0.6113152941176471, 0.9734964705882353, 0.43935882352941175    ,
        0.6089576470588235, 0.9715482352941177, 0.43822941176470587    ,
        0.6066, 0.9696, 0.4371,
        0.6066, 0.9696, 0.4371,
    };

    int index = lround(floor(intensity * 255));
    float inter = intensity * 255 - index;
    float r = colorTable[index * 3] * (1 - inter) + colorTable[(index + 1) * 3] * inter;
    float g = colorTable[index * 3+1] * (1 - inter) + colorTable[(index + 1) * 3+1] * inter;
    float b = colorTable[index * 3+2] * (1 - inter) + colorTable[(index + 1) * 3+2] * inter;
    return (lround(floor(b * 255)) << 16) + (lround(floor(g * 255)) << 8) + (lround(floor(r * 255)));
}

void UpdateColor() {
    int m = 0;
    for (int i = 0; i < splitCount * splitCount; i++)
        if (densityArray[i] > m)
            m = densityArray[i];
    for (int i = 0; i < splitCount; i++) {
        for (int j = 0; j < splitCount; j++) {
            float density = (densityArray[i*splitCount+j] / ((float)m));
            //density = (sin(M_PI*(density - 1 / 2)) + 1) / 2;
            float intensity = 1 - density;
            intensity = intensity * intensity;
            intensity = intensity * intensity;
            intensity = intensity * intensity;
            int intensityInt = lround(floor(intensity * 255));
            //int rgb = lround(floor(density * 255));
            //rgb = 255;
            colorArray[j * splitCount + i] = (0xFF << 24) + ((intensityInt) << 16) + (intensityInt << 8) + (intensityInt);
        }
    }
}

int CoordToArr(int x, int y) {
    int xCoord = lround(floor((((float)x) / (N * RestrictionK)) * splitCount));
    int yCoord = lround(floor((((float)y) / (N * RestrictionK)) * splitCount));
    return xCoord*splitCount+yCoord;
}
int PermToArr(int i) {
    return CoordToArr(i, Perm[i]);
}
void ReconstructCount() {
    for (int i = 0; i < splitCount * splitCount; i++)
        densityArray[i] = 0;

    for (int i = 0; i < N * RestrictionK; i++)
        densityArray[PermToArr(i)]++;
    UpdateColor();
}

void ReconstructSwap(int i, int j) {
    densityArray[CoordToArr(i, Perm[j])]++;
    densityArray[CoordToArr(j, Perm[i])]++;
    densityArray[CoordToArr(i, Perm[i])]--;
    densityArray[CoordToArr(j, Perm[j])]--;
}

void InitDrawings()
{
    splitCount = 350*s;
    densityArray = new int[splitCount * splitCount];
    colorArray = new int[splitCount * splitCount];
}

void CreatePerms()
{
    RestrictionK = 10;


    monotoneRestriction.X = new unsigned int[RestrictionK];
    monotoneRestriction.Y = new unsigned int[RestrictionK];


    restricion.k = RestrictionK;
    restricion.I = new bool[RestrictionK * RestrictionK];
    for (int i = 0; i < RestrictionK * RestrictionK; i++)
        restricion.I[i] = true;

    Count = 0;
    avg1 = 0;
    avg2 = 0;

    PermDirect = new unsigned int[N* RestrictionK];
    PermHasting = new unsigned int[N * RestrictionK];

    for (int i = 0; i < N * RestrictionK; i++) {
        PermDirect[i] = 0;
        PermHasting[i] = 0;
    }

    if (isDirectSample)
        Perm = PermDirect;
    else
        Perm = PermHasting;

    device = RandDevice::RandomSeed();
    //RandDevice::SetSeed(1798297);

    g_pFT = new FenwickTree(N * RestrictionK);
}

int InvDiff(int i, int j) {
    int origI = Perm[i];
    int origJ = Perm[j];

    int a, b;
    if (origI < origJ) {
        a = origI;
        b = origJ;
    } else {
        a = origJ;
        b = origI;
    }

    int l0 = 0, l1 = 0, l2 = 0;

    for (int k = i + 1; k < j; k++) {
        if (Perm[k] < a)
            l0++;
        else if (Perm[k] > b)
            l2++;
        else
            l1++;
    }

    if (origI < origJ) {
        int changeI = l2 + l1 - l0;
        int changeJ = -l2 + l1 + l0;
        return changeI + changeJ + 1;
    }

    int changeI = l2 - l1 - l0;
    int changeJ = -l2 - l1 + l0;
    return changeI + changeJ - 1;
}



void Shuffle() {
    int i, j;
    do
    {
        i = device.UniformN(0, N* RestrictionK -1);
        j = device.UniformN(0, N* RestrictionK -1);
        if (j < i) {
            int temp = i;
            i = j;
            j = temp;
        }
    } while ((!restricion.IsIn(N, i, Perm[j])) ||
        (!restricion.IsIn(N, j, Perm[i])));

    float curQ = exp(-r / N);



    int d = InvDiff(i, j);
    float acceptP;

    if (metropolisAccept) {
        float r = exp(log(curQ) * d);
        acceptP = min(1, r);
    }
    else {
        float r = exp(-log(curQ) * d);
        acceptP = 1 / (1 + r);
    }


    if (device.Bernoulli(acceptP)) {
        ReconstructSwap(i, j);
        int temp = Perm[i];
        Perm[i] = Perm[j];
        Perm[j] = temp;
        ShuffleSuccess++;
    }
}

bool InternalRecurse(bool *res, int size, int i, int *output)
{
    if (i >= size)
        return true;
    for (int j = 0; j <size ; j++) {
        if (!res[i * size + j])
            continue;

        bool collide = false;
        for (int k = 0; k < i; k++)
        {
            if (output[k] == j) {
                collide = true;
                break;
            }
        }
        if (collide)
            continue;

        output[i] = j;

        if (InternalRecurse(res, size, i + 1, output))
            return true;
    }
    return false;
}
bool FindPermanence(bool *res, int size, int *output)
{
    return InternalRecurse(res, size, 0, output);
}

void Resample()
{
    device.SetQ(q);

    float t1 = 0, t2 = 0;

    if (!restricion.IsMonotone()) {
        for (int i = 0; i < N * RestrictionK; i++)
            PermDirect[i] = 0;
        ReconstructCount();
        return;
    }

    int *output = new int[RestrictionK];
    ValidRestriction = FindPermanence(restricion.I, RestrictionK, output);
    delete[] output;
    if (!ValidRestriction) {
        ReconstructCount();
        return;
    }
    

    restricion.FillMonoRestrict(&monotoneRestriction);

    MonotoneSampling(monotoneRestriction, N, q, PermDirect, device, g_pFT, &t1, &t2);
    ReconstructCount();

    avg1 = avg1 * (Count / (Count + 1.0f)) + t1 / (Count+1);
    avg2 = avg2 * (Count / (Count + 1.0f)) + t2 / (Count+1);
    Count++;
}





void ShufflingPrep() {
    hastingValid = true;

    int *Filled = new int[RestrictionK];
    ValidRestriction = FindPermanence(restricion.I, RestrictionK, Filled);

    if (ValidRestriction)
        for (int i = 0; i < RestrictionK; i++)
            for (int j = 0; j < N; j++)
                PermHasting[i * N + j] = Filled[i] * N+j;

    delete[] Filled;

    ReconstructCount();
}
void CleanupPerm()
{
    delete g_pFT;
    delete[] PermHasting;
    delete[] monotoneRestriction.Y;
    delete[] PermDirect;
    delete[] monotoneRestriction.X;
}
void IncreaseN(int deltaN)
{
    CleanupPerm();
    Count = 0;
    avg1 = 0;
    avg2 = 0;
    ShuffleCount = 0;
    ShuffleSuccess = 0;
    avgDraw = 0;
    drawCount = 0;
    ShuffleTime = 0;
    if (N+deltaN > 0)
        N += deltaN;
    CreatePerms();
}
void RepeatFunction()
{
    if (isDirectSample)
        Resample();
    else
    {
        if (ValidRestriction) {
            for (int _ = 0; _ < 10000; _++) {
                auto start = std::chrono::high_resolution_clock::now();
                Shuffle();
                auto end = std::chrono::high_resolution_clock::now();

                std::chrono::duration<float> elapsed = end - start;

                ShuffleTime = ShuffleTime * (ShuffleCount / (ShuffleCount + 1.0f)) + elapsed.count() / (ShuffleCount + 1);
                ShuffleCount++;
            }
            UpdateColor();
        }
    }
    Refresh(g_hWnd);
}
void UpdateQ_R() {
    r = SliderValue * SliderRight + (1 - SliderValue) * SliderLeft;
    q = exp(-r / N);
    ShuffleCount = 0;
    ShuffleSuccess = 0;
    hastingValid = false;

    if (isDirectSample)
        Resample();
    else
        ShufflingPrep();
}

std::vector<std::wstring> GetDebugTextLines()
{
    using std::wstring;
    using std::to_wstring;


    std::vector<wstring> queueStrings;


    wstring displayText = Text("Display info (F): ");
    switch (textOption) {
    case None:
        displayText += Text("None");
        break;
    case Essential:
        displayText += Text("Essential");
        break;
    case Verbose:
        displayText += Text("Verbose");
        break;
    }
    queueStrings.push_back(displayText);



    if (textOption == None)
        return queueStrings;

    queueStrings.push_back(Text("N=") + to_wstring(N) + (Text(", 1 to increase, 2 to decrease")));


    queueStrings.push_back(Text("r=") + to_wstring(r));

    queueStrings.push_back(Text("q=") + to_wstring(exp(-r / N)));

    if (!ValidRestriction)
        queueStrings.push_back(Text("NOT VALID RESTRICTION"));

    if (restricion.IsMonotone())
        queueStrings.push_back(Text("Monotone"));
    else
        queueStrings.push_back(Text("Not monotone"));


    if (textOption == Verbose) {
        wstring time1;
        if (isDirectSample)
            time1 = (Text("avg sample time: ")) + to_wstring(avg1 + avg2);
        else
            time1 = (Text("avg shuffle time: ")) + to_wstring(ShuffleTime);
        queueStrings.push_back(time1);

        queueStrings.push_back(Text("avg draw time: ") + to_wstring(avgDraw));
    }

    wstring displayModeText;
    if (displayOption)
        displayModeText = Text("display mode (d): dots");
    else
        displayModeText = Text("display mode (d): density");
    queueStrings.push_back(displayModeText);

    wstring restrictionText = Text("Show restriction (s): ");
    if (showRestriction)
        restrictionText += Text("true");
    else
        restrictionText += Text("false");
    queueStrings.push_back(restrictionText);


    wstring sampleModeText;
    if (isDirectSample)
        sampleModeText = Text("sample mode (b): direct");
    else
        sampleModeText = Text("sample mode (b): metropolis hasting");
    queueStrings.push_back(sampleModeText);


    wstring repeatText;
    if (isDirectSample)
        repeatText = Text("resampling");
    else
        repeatText = Text("shuffling");
    repeatText += Text(" (k): ");
    if (DoRepeat)
        repeatText += Text("on");
    else
        repeatText += Text("off");
    queueStrings.push_back(repeatText);

    if (!isDirectSample) {
        wstring metropolisModeText = Text("Acceptance function (M): ");
        if (metropolisAccept)
            metropolisModeText += Text(" min(1, pi(y)/pi(x))");
        else
            metropolisModeText += Text(" 1/(1 + pi(y)/pi(x))");
        queueStrings.push_back(metropolisModeText);

        if (textOption == Verbose) {
            std::wstring num_shuffle = Text("Number of shuffle attempted: ") + std::to_wstring(ShuffleCount);
            std::wstring num_succ_shuffle = Text("Number of shuffle succeeded: ") + std::to_wstring(ShuffleSuccess);

            queueStrings.push_back(num_shuffle);
            queueStrings.push_back(num_succ_shuffle);
        }
    }

    return queueStrings;
}


void DrawDebugTexts(ID2D1HwndRenderTarget *pRenderTarget) {

    
    // gui text left space
    int gl = 20;
    int height = 25;
    int linespace = 0;
    int line = g_nHeight / s - gl;

    int corner = 5;
    int sidespace = 10;
    int topbottomspace = 0;// 5;


    ID2D1SolidColorBrush *pBrush;

    pRenderTarget->CreateSolidColorBrush(ColorF(ColorF::Black), &pBrush);

    auto lines = GetDebugTextLines();

    int maxWidth = 0;

    for (auto text : lines) {
        IDWriteTextLayout *textLayout = NULL;
        g_pWriteFactory->CreateTextLayout(
            text.c_str(),
            text.size(),
            g_pTextFormat,
            g_nWidth,
            g_nHeight,
            &textLayout
        );

        DWRITE_TEXT_METRICS metrics;
        textLayout->GetMetrics(&metrics);
        
        if (maxWidth < metrics.width)
            maxWidth = metrics.width;
        if (height < metrics.height)
            height = metrics.height;

        textLayout->Release();
    }

    pBrush->SetColor(ColorF(179 / 255.0f, 125 / 255.0f, 227 / 255.0f, 0.9f));
    pRenderTarget->FillRoundedRectangle(
        RoundedRect(
            RectF(gl - sidespace,
                  line - lines.size() * height - (lines.size() - 1) * linespace - topbottomspace,
                  gl + maxWidth + sidespace,
                  line + topbottomspace),
            corner,
            corner),
        pBrush);



    pBrush->SetColor(ColorF(ColorF::Black));
    for (auto text : lines) {
        line -= height;
        pRenderTarget->DrawTextW(text.c_str(), text.size(), g_pTextFormat, RectF(gl, line, g_nWidth / s, line + height), pBrush);
        line -= linespace;
    }

    pBrush->Release();
}

bool g_bIsSliding = false;
bool g_bSetFlag = false;


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int xPos = (lParam & 0x0000FFFF);
    int yPos = (lParam >> 16);
    switch (message)
    {
    case WM_ERASEBKGND:
    {
        ID2D1HwndRenderTarget *pRenderTarget = NULL;
        HRESULT hr = g_pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(g_hWnd, SizeU(g_nWidth, g_nHeight)), &pRenderTarget);

        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(ColorF(ColorF::White));

        ID2D1SolidColorBrush *pBrush;

        pRenderTarget->CreateSolidColorBrush(ColorF(ColorF::Red), &pBrush);

        int filter = 1;


        float p;




        p = 1.0f / (RestrictionK * N);
        float pad = 0.8f;

        float drawDuration = 0;
        auto last = std::chrono::high_resolution_clock::now();

        if (displayOption) {
            pBrush->SetColor(ColorF(ColorF::Black));
            for (int i = 0; i < filter * RestrictionK * N; i++) {
                float l = ((float)i) * p;
                float r = ((float)i + 1) * p;
                float t = ((float)Perm[i]) * p;
                float b = ((float)Perm[i] + 1) * p;

                float centerX = (l + r) / 2;
                float centerY = (t + b) / 2;

                pRenderTarget->FillRectangle(
                    RectF(l * g_nWidth / s - pad,
                        t * g_nHeight / s - pad,
                        r * g_nWidth / s + pad,
                        b * g_nHeight / s + pad),
                    pBrush);
                //pRenderTarget->FillEllipse(Ellipse(Point2F(centerX*g_nWidth/s, centerY*g_nHeight/s), 1, 1), pBrush);
            }
        }
        else
        {
            int32_t stride = splitCount * sizeof(int32_t);

            D2D1_PIXEL_FORMAT pixFmt = D2D1::PixelFormat(
                DXGI_FORMAT_R8G8B8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE
            );
            D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(pixFmt);
            ID2D1Bitmap *bmp;

            // 4) Create the bitmap from memory:
            HRESULT hr = pRenderTarget->CreateBitmap(
                D2D1::SizeU(splitCount, splitCount), // size in pixels
                colorArray,                              // pointer to row-major BGRA data
                stride,                                   // bytes per row
                &bmpProps,
                &bmp                         // out ID2D1Bitmap*
            );

            pRenderTarget->DrawBitmap(bmp, RectF(0, 0, g_nWidth / s, g_nHeight / s), 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
            bmp->Release();

            //int m = 0;
            //for (int i = 0; i < splitCount * splitCount; i++)
            //    if (m < densityArray[i])
            //        m = densityArray[i];
            //// block length
            //float bLen = g_nWidth / s / ((float)splitCount);
            //for (int i = 0; i < splitCount; i++) {
            //    for (int j = 0; j < splitCount; j++) {
            //        float density = densityArray[i * splitCount + j] / ((float)m);
            //        float s = 1 - density;
            //        pBrush->SetColor(ColorF(s, s, s, 1));
            //        pRenderTarget->FillRectangle(RectF(bLen * i, bLen * j, bLen * (i + 1), bLen * (j + 1)), pBrush);
            //    }
            //}
        }

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - last;
        drawDuration = elapsed.count();
        avgDraw = avgDraw * (drawCount / (drawCount + 1.0f)) + drawDuration / (drawCount + 1);
        drawCount++;

        if (showRestriction) {
            float px = g_nWidth / s / RestrictionK;
            float py = g_nHeight / s / RestrictionK;
            pBrush->SetColor(ColorF(198 / 255.0f, 177 / 255.0f, 237 / 255.0f, 1));
            for (int i = 0; i < RestrictionK; i++) {
                for (int j = 0; j < RestrictionK; j++) {
                    if (restricion.Get(i, j))
                        continue;

                    float l = i * px;
                    float r = (i + 1) * px;
                    float t = j * py;
                    float b = (j + 1) * py;

                    pRenderTarget->FillRectangle(RectF(l, t, r, b), pBrush);
                }
            }
        }

        //int pRSum = 0;
        //int pCSum = 0;
        //p = 1.0f / (MSize);
        //for (int i = 0; i < monotoneRestriction.dim; i++)
        //{
        //    float l = ((float)pRSum) * p;
        //    float r = ((float)pRSum + monotoneRestriction.X[i]) * p;
        //    float t = ((float)pCSum + monotoneRestriction.Y[i]) * p;
        //    float b = 1;
        //
        //
        //    pRSum += monotoneRestriction.X[i];
        //    pCSum += monotoneRestriction.Y[i];
        //
        //    pRenderTarget->FillRectangle(RectF(l * g_nWidth / s, t * g_nHeight / s, r * g_nWidth / s, b * g_nHeight / s), pBrush);
        //}




        pBrush->SetColor(ColorF(ColorF::Gray));

        pRenderTarget->DrawLine(Point2F(SliderBorder, SliderBorder),
            Point2F(g_nWidth / s - SliderBorder, SliderBorder),
            pBrush, 3.0f);
        pBrush->SetColor(ColorF(ColorF::Black));
        pRenderTarget->FillEllipse(Ellipse(Point2F(SliderBorder + (g_nWidth / s - 2 * SliderBorder) * SliderValue, SliderBorder), SliderR, SliderR), pBrush);
        pRenderTarget->DrawLine(
            Point2F(g_nWidth / s / 2, SliderHeight - 5),
            Point2F(g_nWidth / s / 2, SliderHeight + 5),
            pBrush);

        DrawDebugTexts(pRenderTarget);


        pBrush->Release();
        pRenderTarget->EndDraw();

        pRenderTarget->Release();

        break;
    }
    case WM_EXITSIZEMOVE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);

        g_nWidth = rect.right - rect.left;
        g_nHeight = rect.bottom - rect.top;

        break;
    }
    case WM_LBUTTONDOWN:
    {
        // xPos and yPos are valid here

        // testing code
        g_nX = xPos/s;
        g_nY = yPos/s;

        if (g_nX > SliderBorder && g_nX < g_nWidth / s - SliderBorder &&
            g_nY > SliderBorder - SliderHeight / 2 && g_nY < SliderBorder + SliderHeight / 2)
        {
            isOnSlider = true;
            SliderValue = ((float)g_nX - SliderBorder) / (g_nWidth / s - SliderBorder * 2);
            UpdateQ_R();
            Refresh(hWnd);
            break;
        }

        int indexX = g_nX / (g_nWidth/s / RestrictionK);
        int indexY = g_nY / (g_nHeight/s / RestrictionK);

        g_bIsSliding = true;
        g_bSetFlag = !restricion.Get(indexX, indexY);

        restricion.Set(indexX, indexY, g_bSetFlag);

        DoRepeat = false;
        if (isDirectSample)
            Resample();
        else
            ShufflingPrep();
        Refresh(hWnd);

        break;
    }
    case WM_MOUSEMOVE:
    {
        // xPos and yPos are valid here
        g_nX = xPos/s;
        g_nY = yPos/s;
        if (isOnSlider)
        {
            if (!(g_nX > SliderBorder && g_nX < g_nWidth / s - SliderBorder &&
                g_nY > SliderBorder - SliderHeight / 2 && g_nY < SliderBorder + SliderHeight / 2))
            {
                isOnSlider = false;
                break;
            }
            // testing code
            SliderValue = ((float)g_nX - SliderBorder) / (g_nWidth / s - SliderBorder * 2);
            if (SliderValue < 0)
                SliderValue = 0;
            if (SliderValue > 1)
                SliderValue = 1;
            UpdateQ_R();
            Refresh(hWnd);
        }

        if (g_bIsSliding)
        {
            int indexX = g_nX / (g_nWidth / s / RestrictionK);
            int indexY = g_nY / (g_nHeight / s / RestrictionK);

            if (restricion.Get(indexX, indexY) != g_bSetFlag) {
                restricion.Set(indexX, indexY, g_bSetFlag);
                if (isDirectSample)
                    Resample();
                else
                    ShufflingPrep();
            }

            Refresh(hWnd);
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        // xPos and yPos are valid here
        if (isOnSlider) {
            isOnSlider = false;
        }
        g_bIsSliding = false;
        g_nX = xPos/s;
        g_nY = yPos/s;
        break;
    }
    case WM_RBUTTONUP:
    {
        break;
    }
    case WM_KEYUP:
    {
        switch (wParam)
        {
        case VK_RETURN:
            Refresh(hWnd);
            break;
        case VK_SPACE:
            if (isDirectSample)
                Resample();
            Refresh(hWnd);
            break;
        case 'K':
            DoRepeat = !DoRepeat;
            Refresh(hWnd);
            break;
        case 'B':
            isDirectSample = !isDirectSample;
            if (isDirectSample)
            {
                RepeatInterval = ResampleInterval;
                Perm = PermDirect;
                Resample();
            }
            else
            {
                RepeatInterval = ShuffleInterval;
                Perm = PermHasting;
                ReconstructCount();
                if (!hastingValid)
                    ShufflingPrep();
            }
            Refresh(hWnd);
            break;
        case 'D':
            displayOption = !displayOption;
            drawCount = 0;
            avgDraw = 0;
            Refresh(hWnd);
            break;
        case 'M':
            if (!isDirectSample)
                metropolisAccept = !metropolisAccept;
            break;
        case 'F':
            switch (textOption) {
            case None:
                textOption = Essential;
                break;
            case Essential:
                textOption = Verbose;
                break;
            case Verbose:
                textOption = None;
                break;
            }
            Refresh(hWnd);
            break;
        case 'S':
            showRestriction = !showRestriction;
            Refresh(hWnd);
            break;
        }

        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case '1':
            IncreaseN(100);
            if (isDirectSample)
                Resample();
            else
                ShufflingPrep();
            Refresh(hWnd);
            break;
        case '2':
            IncreaseN(-100);
            if (isDirectSample)
                Resample();
            else
                ShufflingPrep();
            Refresh(hWnd);
            break;
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

    return RegisterClassEx(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    RECT rc = { 0, 0, g_nWidth, g_nHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    //std::wstring w = std::to_wstring(rc.left) + Text(" ")
    //    + std::to_wstring(rc.top) + Text(" ")
    //    + std::to_wstring(rc.bottom) + Text(" ")
    //    + std::to_wstring(rc.right);

    //// -8 -31 708 708
    //
    //// -8 -31 708 708
    // -13 -58 1413 1413

    //MessageBox(NULL, w.c_str(), Text("ERROR"), 0);
    DWORD style = WS_OVERLAPPED     // basic, non©\resizable top©\level window
        | WS_CAPTION        // give it a title bar
        | WS_SYSMENU        // include the system menu (close/minimize)
        | WS_MINIMIZEBOX;
    g_hWnd = CreateWindow(szWindowClass, szTitle, style,
        CW_USEDEFAULT, 0, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

    
    if (!g_hWnd)
        return FALSE;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

#include <shellscalingapi.h>

#pragma comment (lib, "Shcore.lib")

#include <iostream>


#include "AVLTree.h"

int countRange(int *perm, int a, int b, int L, int R) {
    int count = 0;
    for (int i = L; i < R; i++) {
        if (a <= perm[i] && perm[i] < b)
            count++;
    }
    return count;
}

int countRange(unsigned int *perm, int a, int b, int L, int R) {
    int count = 0;
    for (int i = L; i < R; i++) {
        if (a <= perm[i] && perm[i] < b)
            count++;
    }
    return count;
}

int YetAnotherTest()
{
    int Num = 20;
    FenwickTree ft(Num);
    ft.init(Num);

    for (int i = 0; i < 100; i++)
    {
        int k;
        cin >> k;
        cout << ft.removeIth(k) << endl;
    }
    int k;
    cin >> k;

    return 0;
}
int AVLTest()
{
    AVLTree avlTree;

#define Num 2000


    device = RandDevice::SetSeed(1798297);
    int *permutation = new int[Num];
    FenwickTree ft(Num);
    ft.init(Num);
    for (int i = 0; i < Num; i++) {
        int removeIndex = device.UniformN(1, Num - i);
        permutation[i] = ft.removeIth(removeIndex)-1;
        //std::cout << i << ": " << permutation[i] << std::endl;
    }

    avlTree.Init(permutation, Num);

    int nextSeed = device.UniformN(1, 100000000);
    RandDevice::DeleteDevice(device);


    while (true) {
        std::cout << "New round!" << std::endl;
        float opTime = 0;
        float origTime = 0;
        int sampleCount = 0;
        int sampleCountTotal = 1;
        int cOp = 0, cOrig = 0;

        device = RandDevice::SetSeed(nextSeed);

        auto startOrig = std::chrono::high_resolution_clock::now();
        for (int _ = 0; _ < sampleCountTotal; _++) {

            int a = 0, b = 0;

            while (a == b) {
                a = device.UniformN(1, Num);
                b = device.UniformN(1, Num);

                if (a > b) {
                    int temp = b;
                    b = a;
                    a = temp;
                }
            }

            int orig = countRange(permutation, a, b, 0, Num);
            cOrig += orig;
        }
        auto endOrig = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedOrig = endOrig - startOrig;


        RandDevice::DeleteDevice(device);
        device = RandDevice::SetSeed(nextSeed);


        auto startOp = std::chrono::high_resolution_clock::now();
        for (int _ = 0; _ < sampleCountTotal; _++) {

            int a = 0, b = 0;

            while (a == b) {
                a = device.UniformN(1, Num);
                b = device.UniformN(1, Num);

                if (a > b) {
                    int temp = b;
                    b = a;
                    a = temp;
                }
            }


            int op = avlTree.Count(b) - avlTree.Count(a);
            cOp += op;
        }
        auto endOp = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float> elapsedOp = endOp - startOp;

        //if (op != orig)
        //{
        //    std::cout << "WRONG OUTPUT FOR a=" << a << " b=" << b << " L=" << L << " R=" << R << "!" << std::endl;
        //    return 0;
        //}

        origTime = elapsedOrig.count() / sampleCountTotal;
        opTime = elapsedOp.count() / sampleCountTotal;

        std::cout << "With " << sampleCountTotal << " samples, the original time is " << origTime << " and the optimized time is " << opTime << std::endl;
        std::cout << "The speed improvement is about " << origTime / opTime << " times" << std::endl;
        std::cout << "c against evil optimization" << cOrig << " " << cOp << std::endl;
        if (cOrig != cOp)
        {
            int dummy;
            std::cout << "Fucking stupid dumbass piece of shit" << std::endl;
            std::cin >> dummy;
        }

        int changeIndex = 0;
        int changeTo = 0;
        do
        {
            changeIndex = device.UniformN(0, Num-1);
            changeTo = device.UniformN(1, 2*Num);
            bool Failed = false;
            for (int i = 0; i < Num; i++)
            {
                if (i == changeIndex)
                    continue;
                if (changeTo == permutation[i])
                {
                    Failed = true;
                    break;
                }
            }
            if (!Failed)
                break;
        } while (true);

        std::cout << "Changing index " << changeIndex << " to " << changeTo << "!" << std::endl;
        if (changeIndex == 16 && changeTo == 19)
        {
            int k = 0;
        }
        avlTree.Change(changeIndex, changeTo);
        permutation[changeIndex] = changeTo;
        


        nextSeed = device.UniformN(1, 100000000);
        RandDevice::DeleteDevice(device);
    }





    
    return 0;
}

#include "SegmentTree.h"

int main(int argc, char *argv[]) {
    //int kDebug = __builtin_popcountll(0xFFFFFFFF);

    device = RandDevice::SetSeed(1798297);
#define Num 2000000
    unsigned int *permutation = new unsigned int[Num];
    FenwickTree ft(Num);
    ft.init(Num);

    for (int i = 0; i < Num; i++) {
        permutation[i] = ft.removeIth(device.UniformN(1, Num - i));
        //std::cout << i << ": " << permutation[i] << std::endl;
    }

    SegmentTree *tree = new SegmentTree();
    tree->Create(permutation, Num, 10000);

    //tree->root->bitVector.PRINT();


    int nextSeed = device.UniformN(1, 100000000);
    RandDevice::DeleteDevice(device);
    float avgSwitchSpeed = 0;
    float avgOrigSpeed = 0;
    float avgOpSpeed = 0;
    int numTrials = 0;
    
    while (true) {
        float opTime = 0;
        float origTime = 0;
        int sampleCount = 0;
        int sampleCountTotal = 3;
        int cOp = 0, cOrig = 0;

        device = RandDevice::SetSeed(nextSeed);

        auto startOrig = std::chrono::high_resolution_clock::now();
        for (int _ = 0; _ < sampleCountTotal; _++) {

            int L = device.UniformN(0, Num-1);
            int R = device.UniformN(0, Num);

            if (L > R) {
                int temp = L;
                L = R;
                R = temp;
            }

            int a = 0, b = 0;

            while (a == b) {
                a = device.UniformN(0, Num-1);
                b = device.UniformN(0, Num);

                if (a > b) {
                    int temp = b;
                    b = a;
                    a = temp;
                }
            }

            int orig = countRange(permutation, a, b, L, R);
            cOrig += orig;
        }
        auto endOrig = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedOrig = endOrig - startOrig;


        RandDevice::DeleteDevice(device);
        device = RandDevice::SetSeed(nextSeed);


        auto startOp = std::chrono::high_resolution_clock::now();
        for (int _ = 0; _ < sampleCountTotal; _++) {

            int L = device.UniformN(0, Num-1);
            int R = device.UniformN(0, Num);

            if (L > R) {
                int temp = L;
                L = R;
                R = temp;
            }

            int a = 0, b = 0;

            while (a == b) {
                a = device.UniformN(0, Num-1);
                b = device.UniformN(0, Num);

                if (a > b) {
                    int temp = b;
                    b = a;
                    a = temp;
                }
            }


            int op = tree->Range(L, R, a, b);
            cOp += op;
        }
        auto endOp = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float> elapsedOp = endOp - startOp;

        if (cOp != cOrig)
        {
            int k;
            std::cout << "WRONG OUTPUT" << std::endl;
            cin>>k;
            return 0;
        }

        origTime = elapsedOrig.count() / sampleCountTotal;
        opTime = elapsedOp.count() / sampleCountTotal;

        std::cout << "With " << sampleCountTotal << " samples, the original time is " << origTime << " and the optimized time is " << opTime << std::endl;
        std::cout << "The speed improvement is about " << origTime / opTime << " times" << std::endl;
        std::cout << "c against evil optimization" << cOrig << cOp << std::endl;

        if (cOrig != cOp)
        {
            int k = 0;
        }

        int changeIndex1 = 0;
        int changeIndex2 = 0;
        while (changeIndex1 == changeIndex2) {

            changeIndex1 = device.UniformN(0, Num - 1);
            changeIndex2 = device.UniformN(0, Num - 1);
        }

        std::cout << "Changing index " << changeIndex1 << " and " << changeIndex2 << "!" << std::endl;
        
        if (changeIndex2 == 292)
        {
            int k = 0;
        }

        if (changeIndex2 == 145)
        {
            int k = 0;
        }

        auto startSwitch = std::chrono::high_resolution_clock::now();
        tree->Switch(changeIndex1, changeIndex2);
        auto endSwitch = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedSwitch = endSwitch - startSwitch;

        avgSwitchSpeed = avgSwitchSpeed * (numTrials / (numTrials + 1.0f)) + elapsedSwitch.count() / (numTrials + 1);
        avgOpSpeed = avgOpSpeed * (numTrials / (numTrials + 1.0f)) + opTime / (numTrials + 1);
        avgOrigSpeed = avgOrigSpeed * (numTrials / (numTrials + 1.0f)) + origTime / (numTrials + 1);

        numTrials++;

        if (numTrials % 1000 == 0)
        {
            cout << endl;
            cout << "running average switching speed: " << avgSwitchSpeed << endl;
            cout << "running average original speed:  " << avgOrigSpeed << endl;
            cout << "running average optimized speed: " << avgOpSpeed << endl;
            int k;
            cin >> k;
        }
        


        nextSeed = device.UniformN(1, 100000000);
        RandDevice::DeleteDevice(device);
    }

    while (true) {
        int a = 7;
        int b = 21;
        int L = 3;
        int R = 10;
        std::cout << "a: ";
        std::cin >> a;
        std::cout << "b: ";
        std::cin >> b;
        std::cout << "L: ";
        std::cin >> L;
        std::cout << "R: ";
        std::cin >> R;
        int n = 0;// tree->Range(L, R, a, b);

        std::cout << n << std::endl;
        std::cout << "Debug count: " << countRange(permutation, a, b, L, R) << std::endl << std::endl;
    }


    delete[] tree;

    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    if (forProf) {
        s = 1;
    }
    else
    {
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

        s = 2;
    }
    g_nHeight = 700 * s;
    g_nWidth = 700 * s;


    CreatePerms();
    InitDrawings();
    Resample();


    if (FAILED(InitD2D()))
    {
        MessageBox(NULL, Text("Failed creating D2D devices!"), Text("ERROR"), 0);
        return FALSE;
    }

    // Initialize global strings
    szTitle = Text("Title");
    szWindowClass = Text("Animation2D");
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        MessageBox(NULL, Text("Failed creating window!"), Text("ERROR"), 0);
        return FALSE;
    }


    Refresh(g_hWnd);

    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    auto last = std::chrono::high_resolution_clock::now();


    // Main message loop:
    while (msg.message != WM_QUIT)
    {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - last;
        if (elapsed.count() >= RepeatInterval)
        {
            if (DoRepeat)
                RepeatFunction();
            last = std::chrono::high_resolution_clock::now();
        }
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CleanupPerm();

    CleanUpD2D();
    _CrtDumpMemoryLeaks();

    return (int)msg.wParam;
}