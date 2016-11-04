/**
 * \file
 *         interceptor.h
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */


#ifndef INTERCEPTOR_H_
#define INTERCEPTOR_H_


/**
 * The structure of an interceptor driver.
 */
struct interceptor_driver {
  char *name;
  
  void (* init)(void);
  
  void (* send)(mac_callback_t sent_callback, void *ptr);
   
  void (* input)(void);
};

#endif /* INTERCEPTOR_H_ */

