#include "rRadialObjectIdentifier.h"

using namespace rapio;
//Local namespace for subroutines used by the class:
// useful knowledge:
// a value of 0 is "no_data" 
// a value of -1 is a temporaty object (something to be assigned an object id)
// a value > 0 is already an object
//
// flood_fill: computes adjacent objects and assigns them the same id
// object growth: grows the objects by one ring
// size_of_object: computes the size of the object id
// flip_object_id: changes all object ids =X to Y
namespace {

// Iterative Depth-First Search (Safe from Stack Overflow)
void flood_fill(size_t start_r, size_t start_g, float object_label, float temp_obj_id, std::shared_ptr<RadialSet> output) {
    auto& outData = output->getFloat2DRef();
    const size_t numRadials = output->getNumRadials();
    const size_t numGates = output->getNumGates();

    // Heap-allocated stack to replace recursion
    //  uses a pair of r,g as indexes
    std::vector<std::pair<size_t, size_t>> stack;
    //just a guess.
    stack.reserve(100);
    
    // Seed the first pixel
    stack.push_back({start_r, start_g});
    outData[start_r][start_g] = object_label;

    while (!stack.empty()) {
        auto current = stack.back(); //this just handles the last one added first
        stack.pop_back(); //get's the current pair (r,g) of coords
        
        size_t r = current.first;
        size_t g = current.second;

        // 1. Check Up (Further out in range)
        if (g + 1 < numGates) {
            if (outData[r][g + 1] == temp_obj_id) {
                outData[r][g + 1] = object_label; //found one; add one to check
                stack.push_back({r, g + 1});//pushing this on the stack will
                                            //force the code to check the locations
                                            //around it. 
            }
        }
        
        // 2. Check Down (Closer in range)
        if (g > 0 && outData[r][g - 1] == temp_obj_id) {
            outData[r][g - 1] = object_label; //found one; add one to check
            stack.push_back({r, g - 1});
        }

        // 3. Check Right / Clockwise (Handle polar radial wraparound)
        size_t r_cw = (r + 1);
        if ( r_cw == numRadials ) {
            r_cw = 0;
        }
        if (outData[r_cw][g] == temp_obj_id) {
            outData[r_cw][g] = object_label;
            stack.push_back({r_cw, g});
        }

        // 4. Check Left / Counter-Clockwise (Handle polar radial wraparound)
        size_t r_ccw = (r == 0) ? (numRadials - 1) : (r - 1);
        if (outData[r_ccw][g] == temp_obj_id) {
            outData[r_ccw][g] = object_label;
            stack.push_back({r_ccw, g});
        }
    }
}


void object_growth(float temp_obj_id, std::shared_ptr<RadialSet> output) {
    auto& outData = output->getFloat2DRef();
    const size_t numRadials = output->getNumRadials();
    const size_t numGates = output->getNumGates();
    
    // 1. Create a single queue for ALL objects
    std::queue<std::pair<size_t, size_t>> queue;
    
    // 2. Seed the queue with every pixel that currently belongs to an object
    for (size_t r = 0; r < numRadials; ++r) {
        for (size_t g = 0; g < numGates; ++g) {
            if (outData[r][g] > 0.0f) { // If it already has an object ID
                queue.push({r, g});
            }
        }
    }

    //for each member of the queue try and add the neighboring pixel to our object. 
    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();//removes first item
    
        size_t r = current.first;
        size_t g = current.second;
        float current_object_id = outData[r][g]; 

        // Helper lambda to check and claim a neighbor
        auto claim_neighbor = [&](size_t n_r, size_t n_g) {
            // If the neighbor is marked as temp_obj_id, claim it and queue it for the next layer
            if (outData[n_r][n_g] == temp_obj_id) {
                outData[n_r][n_g] = current_object_id;
                queue.push({n_r, n_g});
            }
            // If it already has a value (> 0.0f), another object got here first.
            // We do nothing, which forms a boundary where they meet.
        };
    
        // 1. Check Up (Further out in range)
        if (g + 1 < numGates) claim_neighbor(r, g + 1);
        
        // 2. Check Down (Closer in range)
        if (g > 0) claim_neighbor(r, g - 1);
    
        // 3. Check Right / Clockwise (Handle polar radial wraparound)
        size_t r_cw = (r + 1);
        if ( r_cw == numRadials ) {
            r_cw = 0;
        }
        claim_neighbor(r_cw, g);
    
        // 4. Check Left / Counter-Clockwise (Handle polar radial wraparound)
        size_t r_ccw = (r == 0) ? (numRadials - 1) : (r - 1);
        claim_neighbor(r_ccw, g);
    } 
}

size_t size_of_object(float obj_id, std::shared_ptr<RadialSet> output) {
  if (!output) return 0;

  auto& outData = output->getFloat2DRef();

  const size_t numRadials = output->getNumRadials();
  const size_t numGates = output->getNumGates();

  size_t count = 0;
  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      if (outData[r][g] == obj_id) count++; 
    }
 }
 return count;
}

void flip_object_id( float old_id, float new_id, std::shared_ptr<RadialSet> output) {
  auto& outData = output->getFloat2DRef();

  const size_t numRadials = output->getNumRadials();
  const size_t numGates = output->getNumGates();

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      if (outData[r][g] == old_id) outData[r][g] = new_id;
    }
  }
} 
    
} // end anonymous namespace

std::shared_ptr<RadialSet> RadialObjectIdentifier::ObjectID_Single_Threshold(std::shared_ptr<RadialSet> input, float threshold) {
  if (!input) return nullptr;

  // Clone the input to preserve spatial dimensions and metadata
  auto output = input->Clone();
  
  // Update metadata for the new object radial set
  output->setTypeName(input->getTypeName() + "SingleThreshObjects");
  output->setDataAttributeValue("ColorMap", "KMeans"); 
  output->setUnits("dimensionless");

  // Get raw data references
  auto& inData = input->getFloat2DRef();
  auto& outData = output->getFloat2DRef();

  const size_t numRadials = input->getNumRadials();
  const size_t numGates = input->getNumGates();

  // --- SCIENCE LOGIC HERE ---
  // Simple threshold mask example. Replace with your actual object ID logic 
  // (e.g., connected-component labeling, fuzzy logic, etc.)

  float temp_obj_id = -1.0f;
  float object_label = 1.0f;

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      float val = inData[r][g];
      
      if (Constants::isGood(val) && val >= threshold) {
        outData[r][g] = temp_obj_id; // Tag as an temporary object
      } else {
        outData[r][g] = 0.0f; // Not an object
      }
    }
  }

  
  //Make object tags with flood fill, assigns adjacted temp_object_flags to obj 
  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
    // find a location where the data have 0 and -1 next to one another
      if (outData[r][g] == temp_obj_id ) {
      //This is an unlabeled object:
          flood_fill(r, g, object_label, temp_obj_id, output);
          object_label += 1.0f;
      }
    }
  }

  //This sets the non-objects to missing
  flip_object_id(0, Constants::MissingData, output);

  return output;
}

std::shared_ptr<RadialSet> RadialObjectIdentifier::ObjectID_Multi_Threshold(std::shared_ptr<RadialSet> input, 
                                                                            vector <float> & sorted_thresholds) {
  if (!input) return nullptr;
  //Test inputs. We only want > style inputs. User must flip inputs if they want opposite
  for(size_t t = 0; t < sorted_thresholds.size()-1; ++t) {
    if (sorted_thresholds[t] < sorted_thresholds[t+1]) {
        fLogSevere("RadialObjectIdentifier::ObjectID_Multi_Threshold: Bad input: sorted_tresholds must be largest first");
        return nullptr;
    }
  }
  

  // Clone the input to preserve spatial dimensions and metadata
  auto output = input->Clone();
  
  // Update metadata for the new object radial set
  output->setTypeName(input->getTypeName() + "MultiThreshObjects");
  output->setDataAttributeValue("ColorMap", "KMeans"); 
  output->setUnits("dimensionless");

  // Get raw data references
  auto& inData = input->getFloat2DRef();
  auto& outData = output->getFloat2DRef();

  const size_t numRadials = input->getNumRadials();
  const size_t numGates = input->getNumGates();

  // --- SCIENCE LOGIC HERE ---
  //We flood fill the first threshold
  // Simple threshold mask example. Replace with your actual object ID logic 
  // (e.g., connected-component labeling, fuzzy logic, etc.)
  float temp_obj_id = -1.0f;
  float object_label = 1.0f;

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      float val = inData[r][g];
      
      if (Constants::isGood(val) && val >= sorted_thresholds[0]) {
        outData[r][g] = temp_obj_id; // Tag as an temporary object
      } else {
        outData[r][g] = 0.0f; // Not an object, aka background
      }
    }
  }

  
  //Make unique tags by checking neighboors
  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      if (outData[r][g] == temp_obj_id) {
      //This is an unlabeled object:
          flood_fill(r, g, object_label, temp_obj_id, output);
          object_label += 1.0f;
      }
    }
  }
  // --- First Threshold flood fill complete ---

  //for each additional threshold. Mark the new object locations with -1
  // then run a Multi-Source Breadth-First Search (BFS) using object-growth.
  // finally flood fill any remaining new objects
  for(size_t t = 1; t < sorted_thresholds.size(); ++t) {
      for (size_t r = 0; r < numRadials; ++r) {
        for (size_t g = 0; g < numGates; ++g) {
          float val = inData[r][g];
          if (Constants::isGood(val) && 
              val >= sorted_thresholds[t] && 
              outData[r][g] == 0.0f ) {
            outData[r][g] = temp_obj_id; // Tag as an temporary object
          } 
        }
      }
      //grow these Temporay locations into objects 
      //by adding one "ring" around each object at a time
      object_growth(temp_obj_id, output);

      //finally flood fill any "new" unconnected objects
      for (size_t r = 0; r < numRadials; ++r) {
        for (size_t g = 0; g < numGates; ++g) {
          if (outData[r][g] == temp_obj_id ) {
          //This is an unlabeled object:
              flood_fill(r, g, object_label, temp_obj_id, output);
              object_label += 1.0f;
          }
        }
      }

  }

  //This sets the non-objects to missing
  flip_object_id(0, Constants::MissingData, output);

  return output;
}
//Lakshmanan, Valliappa. Automating the Analysis of Spatial Grids: 
//    A Practical Guide to Data Mining Geospatial Images for Human & Environmental Applications. Springer, 2012.
//6.4 pg 188-191
std::shared_ptr<RadialSet> RadialObjectIdentifier::ObjectID_Hysteresis(std::shared_ptr<RadialSet> input, 
                                                                       vector <float> & sorted_thresholds,
                                                                       vector <size_t> & min_size_in_gates ) 
{
  if (!input) return nullptr;
  //Test inputs. We only want > style inputs. User must flip inputs if they want opposite
  for(size_t t = 0; t < sorted_thresholds.size()-1; ++t) {
    if (sorted_thresholds[t] < sorted_thresholds[t+1]) {
        fLogSevere("RadialObjectIdentifier::ObjectID_Lak_Hysteresis: Bad input: sorted_tresholds must be largest first");
        return nullptr;
    }
  }
  if ( sorted_thresholds.size() != min_size_in_gates.size() ) {
        fLogSevere("RadialObjectIdentifier::ObjectID_Lak_Hysteresis: Bad input: mismatched sorted_thresholds and min_size_in_gates");
        return nullptr;
  }
  

  // Clone the input to preserve spatial dimensions and metadata
  auto output = input->Clone();
  
  // Update metadata for the new object radial set
  output->setTypeName(input->getTypeName() + "HysObjects");
  output->setDataAttributeValue("ColorMap", "KMeans"); 
  output->setUnits("dimensionless");

  // Get raw data references
  auto& inData = input->getFloat2DRef();
  auto& outData = output->getFloat2DRef();

  const size_t numRadials = input->getNumRadials();
  const size_t numGates = input->getNumGates();

  // --- SCIENCE LOGIC HERE ---
  //We flood fill the first threshold
  // Simple threshold mask example. Replace with your actual object ID logic 
  // (e.g., connected-component labeling, fuzzy logic, etc.)
  float temp_obj_id = -1.0f; //Temproary objects, detected locations for new objects
  float object_label = 1.0f; //Current ID, increments when a label is applied
  //float object_background == 0.0f; //This is the background value and "zero" is not a valid
                                     //object id

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      float val = inData[r][g];
      
      if (Constants::isGood(val) && val >= sorted_thresholds[0]) {
        outData[r][g] = temp_obj_id; // Tag as an temporary object
      } else {
        outData[r][g] = 0.0f; // Not an object, part of the background
      }
    }
  }

  
  //Make unique tags by checking neighboors
  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      if (outData[r][g] == temp_obj_id ) {
      //This is an unlabeled object:
          flood_fill(r, g, object_label, temp_obj_id, output);
          object_label += 1.0f;
      }
    }
  }
  //Check each object so that it meets the size requirement for this 
  //Threshold, remove objects that do not
  size_t obj_size = 0;
  for(size_t o=1; o < object_label; ++o ){
      obj_size = size_of_object(o, output);
      if ( obj_size < min_size_in_gates[0] ) {
         flip_object_id(o, 0, output);
      } 
  }
  
  // --- First Threshold flood fill complete, seeds found ---

  //for each additional threshold. Mark the new object locations with -1
  // then run a Multi-Source Breadth-First Search (BFS) using object-growth.
  // finally flood fill any remaining new objects
  for(size_t t = 1; t < sorted_thresholds.size(); ++t) {
      for (size_t r = 0; r < numRadials; ++r) {
        for (size_t g = 0; g < numGates; ++g) {
          float val = inData[r][g];
          if (Constants::isGood(val) && 
              val >= sorted_thresholds[t] && 
              outData[r][g] == 0.0f ) {
            outData[r][g] = temp_obj_id; // Tag as an temporary object
          } 
        }
      }
      //grow these Temporay locations into objects 
      //by adding one "ring" around each object at a time
      object_growth(temp_obj_id, output);

      //finally flood fill any "new" unconnected objects
      for (size_t r = 0; r < numRadials; ++r) {
        for (size_t g = 0; g < numGates; ++g) {
          if (outData[r][g] == temp_obj_id) {
          //This is an unlabeled object:
              flood_fill(r, g, object_label, temp_obj_id, output);
              object_label += 1.0f;
          }
        }
      }

      //additional computations for Lak's Hysterisis:
      //Check each object so that it meets the size requirement for this 
      //Threshold, remove objects that do not
      for(size_t o=1; o < object_label; ++o ){
          obj_size = size_of_object(o, output);
          if ( obj_size < min_size_in_gates[t] ) {
             flip_object_id(o, 0, output);
          } 
      }

  }

  //This sets the non-objects to missing
  flip_object_id(0, Constants::MissingData, output);
  return output;
}
