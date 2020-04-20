#include "pberr.h"
#include "pbmath.h"
#include "genalg.h"
#include "genbrush.h"

#define GAViewerErr GenAlgErr
#define DEFAULT_DIMHISTORYIMG 800

typedef struct Node Node;
typedef struct Node {

  // Epoch
  unsigned long epoch;

  // Position of the node in the image
  VecFloat3D pos;

  // Parents id
  unsigned long parents[2];

  // Id
  unsigned long id;

} Node;

typedef struct {

  // Path to the history file
  char* pathHistory;

  // Path to the history image file
  char* pathHistoryImg;

  // Dimensions of the output image for history
  VecShort2D dimHistoryImg;

  // Loaded history
  GAHistory history;

  // Converted history into GSet of Node, one GSet per epoch
  // Node are sorted according to the id of their father and their id
  GSet* nodes;

  // Nb of epoch in the history
  unsigned long nbEpoch;

} GAViewer;

// Function to create a new GAViewer,
// Return a pointer to the new GAViewer
GAViewer* GAViewerCreate(void);

// Function to free the memory used by the GAViewer 'that'
void GAViewerFree(GAViewer** const that);

// Process the prior arguments from the command line
// Return true if the arguments were correct, else false
bool GAViewerProcessPriorCmdLineArguments(
     GAViewer* const that,
           const int argc,
  const char** const argv);

// Process the posterior arguments from the command line
// Return true if the arguments were correct, else false
bool GAViewerProcessPosteriorCmdLineArguments(
     GAViewer* const that,
           const int argc,
  const char** const argv);

// Create the image from the history
// Return true if successfull, else false
bool GAViewerHistoryToImg(GAViewer* const that);

// Create the nodes from the history
void GAViewerHistoryToNodes(GAViewer* const that);

// Get the max id from the history
unsigned long GAViewerHistoryGetMaxId(GAViewer* const that);

// Get the nb of epoch from the history
unsigned long GAViewerHistoryGetNbEpoch(GAViewer* const that);

// Function to create a new GAViewer,
// Return a pointer to the new GAViewer
GAViewer* GAViewerCreate(void) {

  // Allocate memory for the CBo
  GAViewer* that =
    PBErrMalloc(
      GAViewerErr,
      sizeof(GAViewer));

  // Init the properties
  that->pathHistory = NULL;
  that->pathHistoryImg = NULL;
  that->dimHistoryImg = VecShortCreateStatic2D();
  VecSet(
    &(that->dimHistoryImg),
    0,
    DEFAULT_DIMHISTORYIMG);
  VecSet(
    &(that->dimHistoryImg),
    1,
    DEFAULT_DIMHISTORYIMG);
  that->history = GAHistoryCreateStatic();
  that->nodes = NULL;
  that->nbEpoch = 0;

  // Return the new GAViewer
  return that;

}

// Function to free the memory used by the GAViewer 'that'
void GAViewerFree(GAViewer** const that) {

  if (that == NULL || *that == NULL) return;

  // Free memory
  if ((*that)->pathHistory != NULL) {

    free((*that)->pathHistory);

  }

  if ((*that)->pathHistoryImg != NULL) {

    free((*that)->pathHistoryImg);

  }

  if ((*that)->nodes != NULL) {

    for (
      unsigned long iEpoch = (*that)->nbEpoch;
      iEpoch--;) {

      GSet* set = (*that)->nodes + iEpoch;
      while(GSetNbElem(set) > 0) {

          Node* node = GSetPop(set);
          free(node);

      };

    }

    free((*that)->nodes);

  }

  GAHistoryFree(&((*that)->history));

  free(*that);

}

// Process the prior arguments from the command line
// Return true if the arguments were correct, else false
bool GAViewerProcessPriorCmdLineArguments(
     GAViewer* const that,
           const int argc,
  const char** const argv) {

#if BUILDMODE == 0
  if (that == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that' is null");
    PBErrCatch(GAViewerErr);

  }

#endif

  // Loop on arguments
  for (
    int iArg = 1;
    iArg < argc;
    ++iArg) {

    int retStrCmp =
      strcmp(
        argv[iArg],
        "-help");
    if (retStrCmp == 0) {

      printf("gaviewer\n");
      printf("[-help] : print the help message\n");
      printf("[-hist] : path to the history file\n");
      printf(
        "[-toImg <path/to/img.tga>] : convert the history to an image " \
        "and save it to the specified path\n");
      printf(
        "[-size <size>] : size in pixel of the generated image " \
        "(square), default is 800px\n");
      printf("\n");

    }

    // If the argument is -hist
    retStrCmp =
      strcmp(
        argv[iArg],
        "-hist");
    if (
      retStrCmp == 0 &&
      iArg < argc - 1) {

      // Try to open the file to check the path
      FILE* stream =
        fopen(
          argv[iArg + 1],
          "r");

      // If the path is correct
      if (stream != NULL) {

        // Update the path to history file
        that->pathHistory = strdup(argv[iArg + 1]);

        // Make sure the history is empty
        GAHistoryFlush(&(that->history));

        // Load the history file
        bool retLoad =
          GAHistoryLoad(
            &(that->history),
            stream);

        // Close the stream
        fclose(stream);

        // If we failed to load the history
        if (retLoad == false) {

          fprintf(
            stderr,
            "Couldn't load the history [%s]\n",
            that->pathHistory);
          return false;

        // Else, we could load the history
        } else {

          // Convert the history into Nodes
          GAViewerHistoryToNodes(that);

          printf(
            "Loaded the history [%s]\n",
            that->pathHistory);

        }

      // Else the path is incorrect
      } else {

        fprintf(
          stderr,
          "The path [%s] is incorrect\n",
          argv[iArg + 1]);
        return false;

      }

    }

    // If the argument is -size
    retStrCmp =
      strcmp(
        argv[iArg],
        "-size");
    if (
      retStrCmp == 0 &&
      iArg < argc - 1) {

      // Decode the size
      int size = atoi(argv[iArg + 1]);

      // If the size is valid
      if (size > 0) {

        // Set the size
        VecSet(
          &(that->dimHistoryImg),
          0,
          size);
        VecSet(
          &(that->dimHistoryImg),
          1,
          size);

      // Else, the size is not valid
      } else {

        fprintf(
          stderr,
          "The size [%s] is incorrect\n",
          argv[iArg + 1]);
        return false;

      }

    }

  }

  // Return the successfull code
  return true;

}

// Process the posterior arguments from the command line
// Return true if the arguments were correct, else false
bool GAViewerProcessPosteriorCmdLineArguments(
     GAViewer* const that,
           const int argc,
  const char** const argv) {

#if BUILDMODE == 0
  if (that == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that' is null");
    PBErrCatch(GAViewerErr);

  }

#endif

  // Loop on arguments
  for (
    int iArg = 1;
    iArg < argc;
    ++iArg) {

    int retStrCmp =
      strcmp(
        argv[iArg],
        "-toImg");
    if (
      retStrCmp == 0 &&
      iArg < argc - 1) {

      // Try to open the file to check the path
      FILE* stream =
        fopen(
          argv[iArg + 1],
          "w");

      // If the path is correct
      if (stream != NULL) {

        // Update the path to history image file
        that->pathHistoryImg = strdup(argv[iArg + 1]);

        // Close the stream
        fclose(stream);

        // Create the image from history
        bool ret = GAViewerHistoryToImg(that);
        if (ret == false) {

          fprintf(
            stderr,
            "Failed to create the history image\n");
          return false;

        }

      // Else the path is incorrect
      } else {

        fprintf(
          stderr,
          "The path [%s] is incorrect\n",
          argv[iArg + 1]);
        return false;

      }

    }

  }

  // Return the successfull code
  return true;

}

// Create the image from the history
// Return true if successfull, else false
bool GAViewerHistoryToImg(GAViewer* const that) {

#if BUILDMODE == 0
  if (that == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that' is null");
    PBErrCatch(GAViewerErr);

  }

  if (that->pathHistoryImg == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that->pathHistory' is null");
    PBErrCatch(GAViewerErr);

  }

#endif

  // Create the GenBrush
  GenBrush* gb = GBCreateImage(&(that->dimHistoryImg));

  // Get the surface of the GenBrush
  GBSurface* surf = GBSurf(gb);

  // Create the objects used to draw the genealogy
  GBPixel colorEpoch = GBColorWhite;
  colorEpoch._rgba[GBPixelRed] = 202;
  colorEpoch._rgba[GBPixelGreen] = 202;
  colorEpoch._rgba[GBPixelBlue] = 202;
  GBInkSolid* inkEpoch = GBInkSolidCreate(&colorEpoch);
  GBPixel colorBirth = GBColorBlack;
  GBInkSolid* inkBirth = GBInkSolidCreate(&colorBirth);
  GBToolPlotter* tool = GBToolPlotterCreate();
  GBEyeOrtho* eye = GBEyeOrthoCreate(GBEyeOrthoViewFront);
  GBHand hand = GBHandCreateStatic(GBHandTypeDefault);

  // Create the layer for the epochs
  GBLayer* layerEpoch = GBSurfaceAddLayer(surf, GBDim(gb));
  GBLayerSetStackPos(layerEpoch, GBLayerStackPosBg);
  GBLayerSetBlendMode(layerEpoch, GBLayerBlendModeOver);

  // Create the layer for the births
  GBLayer* layerBirth = GBSurfaceAddLayer(surf, GBDim(gb));
  GBLayerSetStackPos(layerEpoch, GBLayerStackPosFg);
  GBLayerSetBlendMode(layerEpoch, GBLayerBlendModeOver);

  // Allocate memory for the curves for the epoch
  SCurve** curveEpochs =
    PBErrMalloc(
      GAViewerErr,
      sizeof(SCurve*) * that->nbEpoch);

  // Vector to do computation
  VecFloat3D v = VecFloatCreateStatic3D();

  // Calculate the step along x between two epochs
  float stepXEpoch =
    (float)VecGet(
      &(that->dimHistoryImg),
      0) /
    (float)(that->nbEpoch);

  // Calculate the bottom and top of the epoch curve
  float yMinEpoch =
    0.05 *
    (float)VecGet(
      &(that->dimHistoryImg),
      1);
  float yMaxEpoch =
    0.95 *
    (float)VecGet(
      &(that->dimHistoryImg),
      1);

  // Loop on epochs
  for (
    unsigned int iEpoch = that->nbEpoch;
    iEpoch--;) {

    // Create the curve for the epoch
    curveEpochs[iEpoch] = 
      SCurveCreate(
        1,
        3,
        1);

    // Set the position of the control points
    VecSet(
      &v,
      0,
      stepXEpoch * ((float)iEpoch + 0.5)); 
    VecSet(
      &v,
      1,
      yMinEpoch); 
    SCurveSetCtrl(
      curveEpochs[iEpoch],
      0,
      (VecFloat*)&v);
    VecSet(
      &v,
      1,
      yMaxEpoch); 
    SCurveSetCtrl(
      curveEpochs[iEpoch],
      1,
      (VecFloat*)&v);

    // Create the pod for this curve
    GBObjPod* pod =
      GBAddSCurve(
        gb,
        curveEpochs[iEpoch],
        &eye,
        &hand,
        tool,
        inkEpoch,
        layerEpoch);
    (void)pod;

    // Loop on the birth for this epoch
    

  }

  // Update the GenBrush
  GBUpdate(gb);

  // Save the GenBrush
  GBSetFileName(
    gb,
    that->pathHistoryImg);
  GBRender(gb);

  // Free memory
  for (
    unsigned int iEpoch = that->nbEpoch;
    iEpoch--;) {

    SCurveFree(curveEpochs + iEpoch);

  }
  free(curveEpochs);
  GBInkSolidFree(&inkEpoch);
  GBInkSolidFree(&inkBirth);
  GBToolPlotterFree(&tool);
  GBEyeOrthoFree(&eye);
  GBFree(&gb);

  // Return the success code
  return true;

}

// Get the max id from the history
unsigned long GAViewerHistoryGetMaxId(GAViewer* const that) {

#if BUILDMODE == 0
  if (that == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that' is null");
    PBErrCatch(GAViewerErr);

  }

#endif

  // Get the max id of nodes
  unsigned long maxId = 0;
  GSetIterForward iter =
    GSetIterForwardCreateStatic(&(that->history._genealogy));
  do {

    // Get the current birth
    GAHistoryBirth* birth = GSetIterGet(&iter);

    // Update the max id
    maxId =
      MAX(
        maxId,
        birth->_idChild);

  } while(GSetIterStep(&iter));

  return maxId;

}

// Get the nb of epoch from the history
unsigned long GAViewerHistoryGetNbEpoch(GAViewer* const that) {

#if BUILDMODE == 0
  if (that == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that' is null");
    PBErrCatch(GAViewerErr);

  }

#endif

  // Get the nb of epochs
  unsigned long nbEpoch = 0;
  GSetIterForward iter =
    GSetIterForwardCreateStatic(&(that->history._genealogy));
  do {

    // Get the current birth
    GAHistoryBirth* birth = GSetIterGet(&iter);

    // Update the number of epoch
    nbEpoch =
      MAX(
        nbEpoch,
        birth->_epoch);

  } while(GSetIterStep(&iter));

  return nbEpoch + 1;

}

// Create the nodes from the history
void GAViewerHistoryToNodes(GAViewer* const that) {

#if BUILDMODE == 0
  if (that == NULL) {

    GAViewerErr->_type = PBErrTypeNullPointer;
    sprintf(
      GAViewerErr->_msg,
      "'that' is null");
    PBErrCatch(GAViewerErr);

  }

#endif

  // Get the number of epoch
  that->nbEpoch = GAViewerHistoryGetNbEpoch(that);

  // Allocate memory for the nodes
  that->nodes =
    PBErrMalloc(
      GAViewerErr,
      sizeof(GSet) * (that->nbEpoch));

  // Initialize the sets of nodes per epoch
  for (
    unsigned long iEpoch = that->nbEpoch;
    iEpoch--;) {

    that->nodes[iEpoch] = GSetCreateStatic();

  }

  // Loop on the birth
  GSetIterForward iter =
    GSetIterForwardCreateStatic(&(that->history._genealogy));
  do {

    // Get the current birth
    GAHistoryBirth* birth = GSetIterGet(&iter);

    // Create a node
    Node* node =
      PBErrMalloc(
        GAViewerErr,
        sizeof(Node));

    // Set the properties of the node according to the birth
    node->epoch = birth->_epoch;
    node->id = birth->_idChild;
    node->parents[0] = birth->_idParents[0];
    node->parents[1] = birth->_idParents[1];
    node->pos = VecFloatCreateStatic3D();

    // Add the node to the set of its epoch
    float sortVal =
      (float)(node->parents[0]) +
      1.0 - 1.0 / (float)(node->id);
    GSetAddSort(
      (GSet*)(that->nodes + node->epoch),
      node,
      sortVal);

  } while(GSetIterStep(&iter));

}

int main(
                 int argc,
  const char** const argv) {

  // Declare a variable to memorize the returned code
  int retCode = 0;

  // Create an instance of GAViewer
  GAViewer* viewer = GAViewerCreate();

  // Process the prior arguments from the command line
  bool success =
    GAViewerProcessPriorCmdLineArguments(
      viewer,
      argc,
      argv);

  // If we could process the command line arguments
  if (success == true) {

    // Process the posterior arguments from the command line
    success =
      GAViewerProcessPosteriorCmdLineArguments(
        viewer,
        argc,
        argv);

  // Else, the command line is incorrect
  } else {

    // Set the return code
    retCode = 1;

  }

  // Free the instance of GAViewer
  GAViewerFree(&viewer);

  // Return the success status code
  return retCode;

}
