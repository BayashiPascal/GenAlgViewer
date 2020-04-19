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

  // Normalised position (centered on (0, 0), one epoch unit
  VecFloat2D pos;

  // Parents
  Node* parents[2];

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

  // Converted history into array of node
  Node* nodes;

  // Nb of epoch in the history
  unsigned int nbEpoch;

  // Nb of nodes per epoch
  unsigned int* nbNodePerEpoch;

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
  that->nbNodePerEpoch = NULL;

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

    free((*that)->nodes);

  }

  if ((*that)->nbNodePerEpoch != NULL) {

    free((*that)->nbNodePerEpoch);

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

    CBoErr->_type = PBErrTypeNullPointer;
    sprintf(
      CBoErr->_msg,
      "'that' is null");
    PBErrCatch(CBoErr);

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

    CBoErr->_type = PBErrTypeNullPointer;
    sprintf(
      CBoErr->_msg,
      "'that' is null");
    PBErrCatch(CBoErr);

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

    CBoErr->_type = PBErrTypeNullPointer;
    sprintf(
      CBoErr->_msg,
      "'that' is null");
    PBErrCatch(CBoErr);

  }

  if (that->pathHistoryImg == NULL) {

    CBoErr->_type = PBErrTypeNullPointer;
    sprintf(
      CBoErr->_msg,
      "'that->pathHistory' is null");
    PBErrCatch(CBoErr);

  }

#endif

  // Create the GenBrush
  GenBrush* gb = GBCreateImage(&(that->dimHistoryImg));

  // Get the scale factor for the conversion from epoch unit to pixel
  float scaleEpoch =
    (float)VecGet(
      &(that->dimHistoryImg),
      0) /
    (float)(that->nbEpoch + 2);

  // Draw the epoch circles
  for (
    unsigned int iEpoch = that->nbEpoch;
    iEpoch--;) {

    /*
    Spheroid* spheroid = SpheroidCreate(2);
    Shapoid* shap = (Shapoid*)spheroid;
    VecFloat2D v = VecFloatCreateStatic2D();
    VecSet(&v, 0, 10.0); VecSet(&v, 1, 8.0);
    ShapoidScale(shap, (VecFloat*)&v);
    VecSet(&v, 0, 5.0); VecSet(&v, 1, 5.0);
    ShapoidTranslate(shap, (VecFloat*)&v);


    GBLayer* layer = GBSurfaceAddLayer(surf, &dim);
    GBPixel blue = GBColorBlue;
    GBInkSolid* ink = GBInkSolidCreate(&blue);
    GBToolPlotter* tool = GBToolPlotterCreate();
    GBEyeOrtho* eye = GBEyeOrthoCreate(GBEyeOrthoViewFront);
    GBHand hand = GBHandCreateStatic(GBHandTypeDefault);
    GBObjPod* pod = 
      GBObjPodCreateShapoid(shap, &eye, &hand, tool, ink, layer);
    GSetAppend(&(pod->_handShapoids), shap);
    GBToolPlotterDraw(tool, pod);

    GBObjPodFree(&pod);
    GBToolPlotterFree(&tool);
    GBSurfaceImageFree((GBSurfaceImage**)&surf);
    GBInkSolidFree(&ink);
    GBEyeOrthoFree(&eye);
    */

  }

  // Save the GenBrush
  GBSetFileName(
    gb,
    that->pathHistoryImg);
  GBRender(gb);

  // Free memory
  GBFree(&gb);

  // Return the success code
  return true;

}

// Create the nodes from the history
void GAViewerHistoryToNodes(GAViewer* const that) {

#if BUILDMODE == 0
  if (that == NULL) {

    CBoErr->_type = PBErrTypeNullPointer;
    sprintf(
      CBoErr->_msg,
      "'that' is null");
    PBErrCatch(CBoErr);

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

  // Free memory for the nodes if necessary
  if (that->nodes != NULL) {

    free(that->nodes);

  }

  // Allocate memory for the nodes
  that->nodes =
    PBErrMalloc(
      GAViewerErr,
      sizeof(Node) * (maxId + 1));

  // Loop on the births
  GSetIterReset(&iter);
  do {

    // Get the current birth
    GAHistoryBirth* birth = GSetIterGet(&iter);

    // Create the node
    Node* node = that->nodes + birth->_idChild;
    node->epoch = birth->_epoch;

    //node->parents[0] = that->nodes + birth->_idParents[0];
    //node->parents[1] = that->nodes + birth->_idParents[1];

    // Initialise the position of the node to (nbEpoch, 0)
    node->pos = VecFloatCreateStatic2D();
    VecSet(
      &(node->pos),
      0,
      (float)(node->epoch));

    // Update the number of epoch
    that->nbEpoch =
      MAX(
        that->nbEpoch,
        node->epoch);

  } while(GSetIterStep(&iter));

  // Free memory for the number of nodes per epoch if necessary
  if (that->nbNodePerEpoch != NULL) {

    free(that->nbNodePerEpoch);

  }

  // Allocate memory for the number of nodes per epoch
  that->nbNodePerEpoch =
    PBErrMalloc(
      GAViewerErr,
      sizeof(unsigned int) * (that->nbEpoch + 1));

  // Initialise the number of nodes per epoch
  for (
    unsigned int iEpoch = that->nbEpoch + 1;
    iEpoch--;) {

    that->nbNodePerEpoch[iEpoch] = 0;

  }

  // Count the number of node per epoch
  GSetIterReset(&iter);
  do {

    // Get the current birth
    GAHistoryBirth* birth = GSetIterGet(&iter);

    ++(that->nbNodePerEpoch[birth->_epoch]);

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
